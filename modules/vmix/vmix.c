/**
 * @file vidmix.c Video bridge
 *
 * Copyright (C) 2022 Sebastian Reimers
 */
#include <re_atomic.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <mix.h>
#include "vmix.h"


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

struct hash *vmix_src;
struct list vmix_srcl;
struct hash *vmix_disp;

struct vidmix *vmix_mix;


/*
 * Relay UA events as publish messages to the Broker
 */
static void ua_event_handler(enum bevent_ev ev, struct bevent *event,
			     void *arg)
{
	struct pl r, module = pl_null, myevent = pl_null, sess_id = pl_null;
	struct vidsrc_st *st;
	struct le *le;
	char device[64];
	static uint64_t last_modified;
	uint64_t now;
	(void)arg;

	const char *prm = bevent_get_text(event);

	if (ev != BEVENT_MODULE)
		return;

	/* disabled */
	return;

	now = tmr_jiffies();

	if ((now - last_modified) < 500)
		return;

	/* "aumix,talk,11_audio" */
	pl_set_str(&r, prm);
	re_regex(r.p, r.l, "[^,]+,[^,]+,[^_]+_[~]*", &module, &myevent,
		 &sess_id, NULL);

	if (pl_strcmp(&module, "aumix"))
		return;

	if (pl_strcmp(&myevent, "talk"))
		return;

	re_snprintf(device, sizeof(device), "%r_video", &sess_id);

	st = vmix_src_find(device);

	if (!st)
		return;

	LIST_FOREACH(&vmix_srcl, le)
	{
		struct vidsrc_st *vst = le->data;
		vidmix_source_set_focus(vst->vidmix_src, st->vidmix_src, true);
		warning("set_focus: %s\n", device);
	}

	last_modified = tmr_jiffies();
}


static int video_rec_h(const char *folder, bool enable)
{
	if (folder && enable) {
		return vmix_record_start(folder);
	}

	return vmix_record_close();
}


static uint32_t disp_enable_h(const char *device, bool enable)
{
	struct vidsrc_st *src;

	if (!device)
		return 0;

	src = vmix_src_find(device);
	if (!src)
		return 0;

	vidmix_source_enable(src->vidmix_src, enable);

	return vidmix_source_get_pidx(src->vidmix_src);
}


void vmix_disp_focus(const char *device);
void vmix_disp_focus(const char *device)
{
	struct vidsrc_st *src;
	struct vidsrc_st *main_src;

	src = vmix_src_find(device);
	if (!src)
		return;

	main_src = vmix_srcl.head->data;
	if (!main_src)
		return;

	vidmix_source_set_focus(main_src->vidmix_src, src->vidmix_src, true);
}


void vmix_disp_solo(const char *device);
void vmix_disp_solo(const char *device)
{
	struct vidsrc_st *src;
	struct le *le;

	src = vmix_src_find(device);
	if (!src)
		return;

	LIST_FOREACH(&vmix_srcl, le)
	{
		struct vidsrc_st *srce = le->data;

		if (srce == src)
			vidmix_source_enable(srce->vidmix_src, true);
		else
			vidmix_source_enable(srce->vidmix_src, false);
	}
}


static int module_init(void)
{
	int err;

	err = hash_alloc(&vmix_src, 32);
	err |= hash_alloc(&vmix_disp, 32);
	IF_ERR_GOTO_OUT(err);

	list_init(&vmix_srcl);

	err = vmix_codec_init();
	IF_ERR_GOTO_OUT(err);

	err = vmix_src_init();
	IF_ERR_GOTO_OUT(err);

	err = vidisp_register(&vidisp, baresip_vidispl(), "vmix",
			      vmix_disp_alloc, NULL, vmix_disp_display, 0);
	IF_ERR_GOTO_OUT(err);

	err = vidsrc_register(&vidsrc, baresip_vidsrcl(), "vmix",
			      vmix_src_alloc, NULL);
	IF_ERR_GOTO_OUT(err);

	err = vmix_pktsrc_init();
	IF_ERR_GOTO_OUT(err);

	err = vidmix_alloc(&vmix_mix);
	IF_ERR_GOTO_OUT(err);

	err = bevent_register(ua_event_handler, NULL);

	slmix_set_video_rec_h(slmix(), video_rec_h);
	slmix_set_video_disp_h(slmix(), disp_enable_h);
out:
	return err;
}


static int module_close(void)
{
	vmix_record_close();
	list_flush(&vmix_srcl);
	bevent_unregister(ua_event_handler);
	vidsrc = mem_deref(vidsrc);
	vidisp = mem_deref(vidisp);

	vmix_src  = mem_deref(vmix_src);
	vmix_disp = mem_deref(vmix_disp);

	vmix_mix = mem_deref(vmix_mix);

	vmix_src_close();
	vmix_pktsrc_close();
	vmix_codec_close();

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(vmix) = {
	"vmix",
	"video",
	module_init,
	module_close,
};
