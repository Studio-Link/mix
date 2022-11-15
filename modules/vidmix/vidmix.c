/**
 * @file vidmix.c Video bridge
 *
 * Copyright (C) 2022 Sebastian Reimers
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "vidmix.h"


/**
 * @defgroup vidmix vidmix
 *
 * Video bridge module
 *
 * This module can be used to connect two video devices together,
 * so that all output to VIDISP device is bridged as the input to
 * a VIDSRC device.
 *
 * Sample config:
 *
 \verbatim
  video_display           vidmix,pseudo0
  video_source            vidmix,pseudo0
 \endverbatim
 */


static struct vidisp *vidisp;
static struct vidsrc *vidsrc;

struct hash *vidmix_src;
struct list vidmix_srcl;
struct hash *vidmix_disp;

struct vidmix *vidmix_mix;


/*
 * Relay UA events as publish messages to the Broker
 */
static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct pl r, module = pl_null, event = pl_null, sess_id = pl_null;
	struct vidsrc_st *st;
	struct le *le;
	char device[64];
	static uint64_t last_modified;
	uint64_t now;
	(void)ua;
	(void)call;
	(void)arg;

	if (ev != UA_EVENT_MODULE)
		return;

	/* disabled */
	return;

	now = tmr_jiffies();

	if ((now - last_modified) < 500)
		return;

	// "aumix,talk,11_audio"
	pl_set_str(&r, prm);
	re_regex(r.p, r.l, "[^,]+,[^,]+,[^_]+_[~]*", &module, &event, &sess_id,
		 NULL);

	if (pl_strcmp(&module, "aumix"))
		return;

	if (pl_strcmp(&event, "talk"))
		return;

	re_snprintf(device, sizeof(device), "%r_video", &sess_id);

	st = vidmix_src_find(device);

	if (!st)
		return;

	LIST_FOREACH(&vidmix_srcl, le)
	{
		struct vidsrc_st *vst = le->data;
		vidmix_source_set_focus(vst->vidmix_src, st->vidmix_src, true);
		warning("set_focus: %s\n", device);
	}

	last_modified = tmr_jiffies();
}


static int cmd_record(struct re_printf *pf, void *arg)
{
	struct cmd_arg *carg = arg;
	bool record;
	(void)pf;

	str_bool(&record, carg->prm);

	if (record)
		vidmix_record_start();
	else
		vidmix_record_close();

	return 0;
}


static const struct cmd cmdv[] = {{"vidmix_record", 0, CMD_PRM,
				   "vidmix_record <true,false>", cmd_record}};


static int module_init(void)
{
	int err;

	err = hash_alloc(&vidmix_src, 32);
	err |= hash_alloc(&vidmix_disp, 32);
	IF_ERR_GOTO_OUT(err);

	list_init(&vidmix_srcl);

	err = vidmix_src_init();
	IF_ERR_GOTO_OUT(err);

	err = cmd_register(baresip_commands(), cmdv, ARRAY_SIZE(cmdv));
	IF_ERR_GOTO_OUT(err);

	err = vidisp_register(&vidisp, baresip_vidispl(), "vidmix",
			      vidmix_disp_alloc, NULL, vidmix_disp_display, 0);
	IF_ERR_GOTO_OUT(err);

	err = vidsrc_register(&vidsrc, baresip_vidsrcl(), "vidmix",
			      vidmix_src_alloc, NULL);
	IF_ERR_GOTO_OUT(err);

	err = vidmix_alloc(&vidmix_mix);
	IF_ERR_GOTO_OUT(err);

	err = uag_event_register(ua_event_handler, NULL);
out:
	return err;
}


static int module_close(void)
{
	cmd_unregister(baresip_commands(), cmdv);
	vidmix_record_close();
	list_flush(&vidmix_srcl);
	uag_event_unregister(ua_event_handler);
	vidsrc = mem_deref(vidsrc);
	vidisp = mem_deref(vidisp);

	vidmix_src  = mem_deref(vidmix_src);
	vidmix_disp = mem_deref(vidmix_disp);

	vidmix_mix = mem_deref(vidmix_mix);

	vidmix_src_close();

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(vidmix) = {
	"vidmix",
	"video",
	module_init,
	module_close,
};
