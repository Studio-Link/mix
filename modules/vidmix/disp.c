/**
 * @file vidmix/disp.c vidmix -- display
 *
 * Copyright (C) 2021 Sebastian Reimers
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "vidmix.h"


static void destructor(void *arg)
{
	struct vidisp_st *st = arg;

	if (st->vidsrc)
		st->vidsrc->vidisp = NULL;

	list_unlink(&st->le);
	mem_deref(st->device);
}


int vidmix_disp_alloc(struct vidisp_st **stp, const struct vidisp *vd,
			 struct vidisp_prm *prm, const char *dev,
			 vidisp_resize_h *resizeh, void *arg)
{
	struct vidisp_st *st;
	int err = 0;
	// char device[] = "vidmix";
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
	st->vidsrc = vidmix_src_find(dev);
	if(!st->vidsrc || !st->vidsrc->vidmix_src) {
		err = ENOKEY;
		goto out;
	}

	st->vidsrc->vidisp = st;
	vidmix_source_enable(st->vidsrc->vidmix_src, true);
	hash_append(vidmix_disp, hash_joaat_str(dev), &st->le, st);

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


int vidmix_disp_display(struct vidisp_st *st, const char *title,
			   const struct vidframe *frame, uint64_t timestamp)
{
	int err = 0;
	(void)title;

	if (!st || !frame)
		return EINVAL;

	if (st->vidsrc)
		vidmix_src_input(st->vidsrc, frame, timestamp);
	else {
		debug("vidmix: display: dropping frame (%u x %u)\n",
		      frame->size.w, frame->size.h);
	}

	return err;
}


struct vidisp_st *vidmix_disp_find(const char *device)
{
	return list_ledata(hash_lookup(vidmix_disp, hash_joaat_str(device),
				       list_apply_handler, (void *)device));
}
