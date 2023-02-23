/**
 * @file vidmix/disp.c vidmix -- display
 *
 * Copyright (C) 2021 Sebastian Reimers
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <re_atomic.h>
#include "vmix.h"


static void destructor(void *arg)
{
	struct vidisp_st *st = arg;

	if (st->vidsrc)
		st->vidsrc->vidisp = NULL;

	list_unlink(&st->le);
	mem_deref(st->device);
}


int vmix_disp_alloc(struct vidisp_st **stp, const struct vidisp *vd,
		      struct vidisp_prm *prm, const char *dev,
		      vidisp_resize_h *resizeh, void *arg)
{
	struct vidisp_st *st;
	int err = 0;
	(void)prm;
	(void)resizeh;
	(void)arg;

	if (!stp || !vd || !dev)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	err = str_dup(&st->device, dev);
	if (err)
		goto out;

	/* find the vidsrc with the same device-name */
	st->vidsrc = vmix_src_find(dev);
	if (!st->vidsrc || !st->vidsrc->vidmix_src) {
		err = ENOKEY;
		goto out;
	}

	st->vidsrc->vidisp = st;
	/* vidmix_source_enable(st->vidsrc->vidmix_src, false); */
	hash_append(vmix_disp, hash_joaat_str(dev), &st->le, st);

out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static bool list_apply_handler(struct le *le, void *arg)
{
	struct vidisp_st *st = le->data;

	return 0 == str_cmp(st->device, arg);
}


int vmix_disp_display(struct vidisp_st *st, const char *title,
			const struct vidframe *frame, uint64_t timestamp)
{
	int err = 0;
	(void)title;

	if (!st || !frame)
		return EINVAL;

	if (st->vidsrc)
		vmix_src_input(st->vidsrc, frame, timestamp);
	else {
		debug("vidmix: display: dropping frame (%u x %u)\n",
		      frame->size.w, frame->size.h);
	}

	return err;
}


void vmix_disp_enable(const char *device, bool enable);
void vmix_disp_enable(const char *device, bool enable)
{
	struct vidsrc_st *src;

	if (!device)
		return;

	src = vmix_src_find(device);
	if (!src)
		return;

	vidmix_source_enable(src->vidmix_src, enable);
}


struct vidisp_st *vmix_disp_find(const char *device)
{
	return list_ledata(hash_lookup(vmix_disp, hash_joaat_str(device),
				       list_apply_handler, (void *)device));
}