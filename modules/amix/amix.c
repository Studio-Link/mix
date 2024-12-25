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
static struct hash *amixl    = NULL;
static struct list speakerl  = LIST_INIT;
static struct aumix *aumix   = NULL;

struct ausrc_st {
	struct ausrc_prm prm;
	ausrc_read_h *rh;
	struct auplay_st *st_play;
	void *arg;
	struct amix *amix;
};

struct auplay_st {
	struct auplay_prm prm;
	auplay_write_h *wh;
	int16_t *sampv;
	uint64_t ts;
	RE_ATOMIC int16_t level;
	void *arg;
	struct amix *amix;
};

struct amix {
	struct le le;
	struct le sle;
	struct ausrc_st *src;
	struct auplay_st *play;
	struct aumix_source *aumix_src;
	char *device;
	uint16_t speaker_id;
	bool muted;
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
	struct amix *amix = arg;

	if (!amix || !af || !amix->play)
		return;

	amix->play->wh(af, amix->play->arg);
	af->id = amix->speaker_id;

	re_atomic_rlx_set(&amix->play->level,
			  amix_level(af->sampv, af->sampc / CH));
}


static void mix_handler(const int16_t *sampv, size_t sampc, void *arg)
{
	struct amix *amix = arg;
	struct auframe af;

	if (!amix || !amix->src || !amix->play)
		return;

	auframe_init(&af, amix->src->prm.fmt, (int16_t *)sampv, sampc,
		     amix->src->prm.srate, amix->src->prm.ch);
	af.timestamp = amix->play->ts;

	amix->src->rh(&af, amix->src->arg);
}


static void amix_destructor(void *arg)
{
	struct amix *amix = arg;

	mem_deref(amix->aumix_src);
	hash_unlink(&amix->le);
	list_unlink(&amix->sle);
	mem_deref(amix->device);
}


static int amix_alloc(struct amix **amixp, const char *device)
{
	struct amix *amix;
	int err;

	if (!amixp)
		return EINVAL;

	amix = mem_zalloc(sizeof(struct amix), amix_destructor);
	if (!amix)
		return ENOMEM;

	err = str_dup(&amix->device, device);
	if (err) {
		err = ENOMEM;
		goto out;
	}

	err = aumix_source_alloc(&amix->aumix_src, aumix, mix_handler, amix);
	if (err) {
		err = ENOMEM;
		goto out;
	}

	aumix_source_readh(amix->aumix_src, mix_readh);

	amix->muted = true;

out:
	if (err) {
		mem_deref(amix);
		return err;
	}

	*amixp = amix;
	hash_append(amixl, hash_joaat_str(amix->device), &amix->le, amix);

	return 0;
}


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	if (!st)
		return;

	if (st->amix)
		st->amix->src = NULL;

	aumix_source_enable(st->amix->aumix_src, false);

	mem_deref(st->amix);
}


static bool dev_cmp_h(struct le *le, void *arg)
{
	struct amix *amix = le->data;

	return 0 == str_cmp(amix->device, arg);
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	struct amix *amix = NULL;
	struct le *le;
	int err = 0;

	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->prm = *prm;
	st->rh	= rh;
	st->arg = arg;

	/* setup if auplay is started before ausrc */
	le = hash_lookup(amixl, hash_joaat_str(device), dev_cmp_h,
			 (void *)device);
	if (le) {
		amix = le->data;

		st->amix = mem_ref(amix);

		aumix_source_mute(amix->aumix_src, amix->muted);
		aumix_source_enable(amix->aumix_src, true);

		goto out;
	}

	err = amix_alloc(&amix, device);
	if (err)
		goto out;

	st->amix = amix;

out:
	if (err)
		mem_deref(st);
	else {
		amix->src = st;
		*stp	  = st;
	}

	return err;
}


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	aumix_source_enable(st->amix->aumix_src, false);

	mem_deref(st->sampv);
	if (st->amix)
		st->amix->play = NULL;
	mem_deref(st->amix);
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	struct amix *amix = NULL;
	struct le *le;
	int err = 0;

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

	st->prm = *prm;
	st->wh	= wh;
	st->arg = arg;

	/* setup if ausrc is started before auplay */
	le = hash_lookup(amixl, hash_joaat_str(device), dev_cmp_h,
			 (void *)device);
	if (le) {
		amix = le->data;

		st->amix = mem_ref(amix);

		aumix_source_mute(amix->aumix_src, amix->muted);
		aumix_source_enable(amix->aumix_src, true);

		goto out;
	}

	err = amix_alloc(&amix, device);
	if (err)
		goto out;

	st->amix = amix;

out:
	if (err)
		mem_deref(st);
	else {
		amix->play = st;
		*stp	   = st;
	}

	return err;
}


void amix_mute(const char *dev, bool mute, uint16_t id);
void amix_mute(const char *dev, bool mute, uint16_t id)
{
	struct le *le;

	le = hash_lookup(amixl, hash_joaat_str(dev), dev_cmp_h, (void *)dev);
	if (!le)
		return;

	struct amix *amix = le->data;

	aumix_source_mute(amix->aumix_src, mute);
	amix->muted = mute;
	if (id) {
		amix->speaker_id = id;

		if (!list_contains(&speakerl, &amix->sle))
			list_append(&speakerl, &amix->sle, amix);
	}
}


static uint16_t talk_detection_h(void)
{
	struct le *le;
	uint16_t speaker_id = 0;
	int16_t max	    = 0;

	LIST_FOREACH(&speakerl, le)
	{
		struct amix *amix = le->data;
		if (amix->muted || !amix->play)
			continue;

		int16_t level = re_atomic_rlx(&amix->play->level);

		if (level >= max) {
			max	   = level;
			speaker_id = amix->speaker_id;
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


void vmix_audio_record(struct auframe *af);
static int module_init(void)
{
	int err;

	hash_alloc(&amixl, 32);

	err = ausrc_register(&ausrc, baresip_ausrcl(), "amix", src_alloc);
	IF_ERR_GOTO_OUT(err);

	err = auplay_register(&auplay, baresip_auplayl(), "amix", play_alloc);
	IF_ERR_GOTO_OUT(err)

	err = aumix_alloc(&aumix, SRATE, CH, PTIME);
	IF_ERR_GOTO_OUT(err);

	aumix_recordh(aumix, amix_record);
	aumix_record_sumh(aumix, vmix_audio_record);

	slmix_set_audio_rec_h(slmix(), audio_rec_h);
	slmix_set_time_rec_h(slmix(), amix_record_msecs);
	slmix_set_talk_detect_h(slmix(), talk_detection_h);

out:
	return err;
}


static int module_close(void)
{
	hash_flush(amixl);

	amixl  = mem_deref(amixl);
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);
	aumix  = mem_deref(aumix);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(amix) = {
	"amix",
	"audio",
	module_init,
	module_close,
};
