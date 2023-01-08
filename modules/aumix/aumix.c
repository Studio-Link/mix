/**
 * @file aumix.c N-1 audio source and play (e.g. for centralized conferences)
 *
 * Copyright (C) 2021 Sebastian Reimers
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aumix.h"

static struct auplay *auplay = NULL;
static struct ausrc *ausrc   = NULL;
static struct list auplayl   = LIST_INIT;
static struct list ausrcl    = LIST_INIT;
static struct aumix *aumix   = NULL;
static struct tmr tmr_level;

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
	int16_t level;
	mtx_t *lock;
};

static int16_t aumix_level(const int16_t *sampv, size_t frames)
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

	if (max_l >= MAX_LEVEL || max_r >= MAX_LEVEL) {

		if (max_l > max_r)
			return max_l;
		else
			return max_r;
	}

	return 0;
}


static void mix_readh(struct auframe *af, void *arg)
{
	struct auplay_st *st_play = arg;

	if (!st_play || !af)
		return;

	st_play->wh(af, st_play->arg);
	af->id = st_play->speaker_id;

	/* mtx_lock(st_play->lock); */
	/* st_play->level = aumix_level(st_play->sampv, sampc / CH); */
	/* mtx_unlock(st_play->lock); */
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
	mem_deref(st->lock);
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

	err = mutex_alloc(&st->lock);
	if (err)
		goto out;

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


void aumix_mute(char *device, bool mute, uint16_t id);
void aumix_mute(char *device, bool mute, uint16_t id)
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


static int mix_debug(struct re_printf *pf, void *arg)
{
	struct le *le;
	(void)arg;
	(void)pf;

	LIST_FOREACH(&auplayl, le)
	{
		struct auplay_st *st = le->data;

		info("aumix: %s, muted: %d\n", st->device, st->muted);
	}

	return 0;
}


int aumix_record_enable(bool enable, char *token);
int aumix_record_enable(bool enable, char *token)
{
	int err = 0;

	if (enable)
		err = aumix_record_start(token);
	else
		aumix_record_close();

	return err;
}


static const struct cmd cmdv[] = {
	{"aumix_debug", 'z', 0, "Debug aumix", mix_debug}};


static void talk_detection(void *arg)
{
	struct le *le;
	const char *sess_id = NULL;
	int16_t max	    = 0;
	(void)arg;

	LIST_FOREACH(&auplayl, le)
	{
		struct auplay_st *st = le->data;
		if (st->muted)
			continue;

		mtx_lock(st->lock);
		if (st->level >= max) {
			max	= st->level;
			sess_id = st->device;
		}
		mtx_unlock(st->lock);
	}

	if (sess_id && max >= MAX_LEVEL)
		module_event("aumix", "talk", NULL, NULL, "%s", sess_id);

	tmr_start(&tmr_level, 1000, talk_detection, NULL);
}


static int module_init(void)
{
	int err;

	err = cmd_register(baresip_commands(), cmdv, ARRAY_SIZE(cmdv));
	IF_ERR_GOTO_OUT(err);

	err = ausrc_register(&ausrc, baresip_ausrcl(), "aumix", src_alloc);
	IF_ERR_GOTO_OUT(err);

	err = auplay_register(&auplay, baresip_auplayl(), "aumix", play_alloc);
	IF_ERR_GOTO_OUT(err)

	err = aumix_alloc(&aumix, SRATE, CH, PTIME);
	IF_ERR_GOTO_OUT(err);

	aumix_recordh(aumix, aumix_record);

	list_init(&auplayl);
	list_init(&ausrcl);
	tmr_init(&tmr_level);
	tmr_start(&tmr_level, 1000, talk_detection, NULL);

out:
	return err;
}


static int module_close(void)
{
	tmr_cancel(&tmr_level);
	cmd_unregister(baresip_commands(), cmdv);
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);
	aumix  = mem_deref(aumix);
	list_flush(&auplayl);
	list_flush(&ausrcl);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(aumix) = {
	"aumix",
	"audio",
	module_init,
	module_close,
};
