/**
 * @file aumix.c N-1 audio source and play (e.g. for centralized conferences)
 *
 * Copyright (C) 2021 Sebastian Reimers
 */
#include <re_atomic.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <mix.h>
#include "amix.h"

static struct auplay *auplay = NULL;
static struct ausrc *ausrc   = NULL;
static struct list auplayl   = LIST_INIT;
static struct list ausrcl    = LIST_INIT;
static struct aumix *aumix   = NULL;

struct ausrc_st {
	struct le le;
	struct ausrc_prm prm;
	ausrc_read_h *rh;
	struct auplay_st *st_play;
	void *arg;
	const char *device;
	bool muted;
	uint16_t speaker_id;
};

struct auplay_st {
	struct le le;
	struct auplay_prm prm;
	auplay_write_h *wh;
	int16_t *sampv;
	struct aumix_source *aumix_src;
	struct ausrc_st *st_src;
	uint16_t speaker_id;
	void *arg;
	uint64_t ts;
	const char *device;
	bool muted;
	RE_ATOMIC int16_t level;
};


static int16_t amix_level(const int16_t *sampv, size_t frames)
{
	int pos	      = 0;
	int16_t max_l = 0, max_r = 0;

	for (size_t frame = 0; frame < frames; frame++) {
		if (sampv[pos] > max_l)
			max_l = sampv[pos];
		if (CH <= 1) {
			pos += 1;
			continue;
		}
		if (sampv[pos + 1] > max_r)
			max_r = sampv[pos + 1];
		pos += CH;
	}

	if (max_l > max_r)
		return max_l;
	else
		return max_r;
}


static void mix_readh(struct auframe *af, void *arg)
{
	struct auplay_st *st_play = arg;

	if (!st_play || !af)
		return;

	st_play->wh(af, st_play->arg);
	af->id = st_play->speaker_id;

	re_atomic_rlx_set(&st_play->level,
			  amix_level(af->sampv, af->sampc / CH));
}


static void mix_handler(const int16_t *sampv, size_t sampc, void *arg)
{
	struct auplay_st *st_play = arg;
	struct auframe af;

	if (!st_play->st_src)
		return;

	auframe_init(&af, st_play->st_src->prm.fmt, (int16_t *)sampv, sampc,
		     st_play->st_src->prm.srate, st_play->prm.ch);
	af.timestamp = st_play->ts;
	st_play->st_src->rh(&af, st_play->st_src->arg);
}


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;
	if (st->st_play && st->st_play->aumix_src)
		aumix_source_enable(st->st_play->aumix_src, false);
	list_unlink(&st->le);
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	struct le *le;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->prm	   = *prm;
	st->rh	   = rh;
	st->arg	   = arg;
	st->device = device;
	st->muted  = true;

	/* setup if auplay is started before ausrc */
	for (le = list_head(&auplayl); le; le = le->next) {
		struct auplay_st *st_play = le->data;

		/* compare struct audio arg */
		if (st->arg == st_play->arg) {
			st_play->st_src = st;
			st->st_play	= st_play;

			if (!st_play->muted) {
				st->muted = st_play->muted;
			}
			aumix_source_mute(st_play->aumix_src, st->muted);
			aumix_source_enable(st_play->aumix_src, true);
			break;
		}
	}

	list_append(&ausrcl, &st->le, st);

	*stp = st;

	return 0;
}


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	mem_deref(st->aumix_src);
	mem_deref(st->sampv);
	list_unlink(&st->le);
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	int err;
	struct le *le;

	if (!stp || !ap || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->sampv = mem_zalloc((SRATE * CH * PTIME / 1000) * sizeof(int16_t),
			       NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	st->prm	   = *prm;
	st->wh	   = wh;
	st->arg	   = arg;
	st->device = device;
	st->muted  = true;

	err = aumix_source_alloc(&st->aumix_src, aumix, mix_handler, st);
	if (err)
		goto out;

	aumix_source_readh(st->aumix_src, mix_readh);

	/* setup if ausrc is started before auplay */
	for (le = list_head(&ausrcl); le; le = le->next) {
		struct ausrc_st *st_src = le->data;

		/* compare struct audio arg */
		if (st->arg == st_src->arg) {
			st_src->st_play = st;
			st->st_src	= st_src;

			if (!st_src->muted) {
				st->muted = false;
			}
			st->speaker_id = st_src->speaker_id;
			aumix_source_mute(st->aumix_src, st->muted);
			aumix_source_enable(st->aumix_src, true);
			break;
		}
	}

	list_append(&auplayl, &st->le, st);

out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


void amix_mute(char *device, bool mute, uint16_t id);
void amix_mute(char *device, bool mute, uint16_t id)
{
	struct le *le;

	info("aumix_mute %s %d %d\n", device, mute, id);
	LIST_FOREACH(&auplayl, le)
	{
		struct auplay_st *st = le->data;

		if (str_cmp(st->device, device))
			continue;

		aumix_source_mute(st->aumix_src, mute);
		st->muted      = mute;
		st->speaker_id = id;

		return;
	}

	/* Fallback if auplay is not started yet */
	LIST_FOREACH(&ausrcl, le)
	{
		struct ausrc_st *st = le->data;

		if (str_cmp(st->device, device))
			continue;

		st->muted      = mute;
		st->speaker_id = id;
	}
}


static uint16_t talk_detection_h(void)
{
	struct le *le;
	uint16_t speaker_id = 0;
	int16_t max	    = 0;
	int16_t level	    = 0;

	LIST_FOREACH(&auplayl, le)
	{
		struct auplay_st *st = le->data;
		if (st->muted)
			continue;

		level = re_atomic_rlx(&st->level);

		if (level >= max) {
			max	   = level;
			speaker_id = st->speaker_id;
		}
	}

	return speaker_id;
}


static int audio_rec_h(const char *folder, bool enable)
{
	if (folder && enable) {
		return amix_record_start(folder);
	}

	return amix_record_close();
}


static int module_init(void)
{
	int err;

	err = ausrc_register(&ausrc, baresip_ausrcl(), "amix", src_alloc);
	IF_ERR_GOTO_OUT(err);

	err = auplay_register(&auplay, baresip_auplayl(), "amix", play_alloc);
	IF_ERR_GOTO_OUT(err)

	err = aumix_alloc(&aumix, SRATE, CH, PTIME);
	IF_ERR_GOTO_OUT(err);

	aumix_recordh(aumix, amix_record);

	list_init(&auplayl);
	list_init(&ausrcl);

	slmix_set_audio_rec_h(slmix(), audio_rec_h);
	slmix_set_time_rec_h(slmix(), amix_record_msecs);
	slmix_set_talk_detect_h(slmix(), talk_detection_h);

out:
	return err;
}


static int module_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);
	aumix  = mem_deref(aumix);
	list_flush(&auplayl);
	list_flush(&ausrcl);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(amix) = {
	"amix",
	"audio",
	module_init,
	module_close,
};
