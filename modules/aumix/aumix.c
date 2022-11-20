/**
 * @file aumix.c N-1 audio source and play (e.g. for centralized conferences)
 *
 * Copyright (C) 2021 Sebastian Reimers
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aumix.h"

enum { PTIME = 20, SRATE = 48000, CH = 1, MAX_LEVEL = 500 };

static struct auplay *auplay	      = NULL;
static struct ausrc *ausrc	      = NULL;
static struct list auplayl	      = LIST_INIT;
static struct list ausrcl	      = LIST_INIT;
static struct aumix *aumix	      = NULL;
struct aumix_source *aumix_record_src = NULL;
static struct tmr tmr_level;

struct ausrc_st {
	struct le le;
	struct ausrc_prm prm;
	ausrc_read_h *rh;
	struct auplay_st *st_play;
	void *arg;
	const char *device;
	bool muted;
};

struct auplay_st {
	struct le le;
	struct auplay_prm prm;
	auplay_write_h *wh;
	int16_t *sampv;
	struct aumix_source *aumix_src;
	struct ausrc_st *st_src;
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

	for (uint16_t frame = 0; frame < frames; frame++) {
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


static void record_handler(const int16_t *sampv, size_t sampc, void *arg)
{
	(void)arg;
	aumix_record((uint8_t *)sampv, sampc * sizeof(int16_t));
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

	auframe_init(&af, st_play->prm.fmt, st_play->sampv, sampc,
		     st_play->prm.srate, st_play->prm.ch);
	af.timestamp = st_play->ts;
	st_play->wh(&af, st_play->arg);

	aumix_source_put(st_play->aumix_src, st_play->sampv, sampc);

	mtx_lock(st_play->lock);
	st_play->level = aumix_level(st_play->sampv, sampc / CH);
	mtx_unlock(st_play->lock);

	st_play->ts += PTIME * 1000;
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
				st->muted = false;
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


static int source_mute(struct re_printf *pf, void *arg)
{
	struct cmd_arg *carg = arg;
	struct le *le;
	struct pl r, device = pl_null, mute_pl = pl_null;
	bool mute;
	int err;
	(void)pf;

	pl_set_str(&r, carg->prm);
	err = re_regex(r.p, r.l, "[^,]+,[~]*", &device, &mute_pl);
	IF_ERR_RETURN(err);

	str_bool(&mute, mute_pl.p);

	LIST_FOREACH(&auplayl, le)
	{
		struct auplay_st *st = le->data;
		struct pl st_device  = pl_null;

		pl_set_str(&st_device, st->device);
		if (pl_cmp(&st_device, &device))
			continue;

		info("aumix_mute %r %d\n", &device, mute);
		aumix_source_mute(st->aumix_src, mute);
		st->muted = mute;
	}

	/* Fallback if auplay is not started yet */
	LIST_FOREACH(&ausrcl, le)
	{
		struct ausrc_st *st = le->data;
		struct pl st_device = pl_null;

		pl_set_str(&st_device, st->device);
		if (pl_cmp(&st_device, &device))
			continue;

		st->muted = mute;
	}

	return err;
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


static int cmd_record(struct re_printf *pf, void *arg)
{
	struct cmd_arg *carg = arg;
	bool record;
	(void)pf;

	str_bool(&record, carg->prm);

	if (record) {
		aumix_record_start();
		aumix_source_alloc(&aumix_record_src, aumix, record_handler,
				   NULL);
		aumix_source_enable(aumix_record_src, true);
	}
	else {
		aumix_record_src = mem_deref(aumix_record_src);
		aumix_record_close();
	}

	return 0;
}


static const struct cmd cmdv[] = {
	{"aumix_mute", 0, CMD_PRM, "aumix_mute <device>,<true,false>}",
	 source_mute},
	{"aumix_debug", 'z', 0, "Debug aumix", mix_debug},
	{"aumix_record", 0, CMD_PRM, "aumix_record <true,false>", cmd_record}};


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