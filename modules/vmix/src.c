/**
 * @file vidmix/src.c vidmix source
 *
 * Copyright (C) 2021-2022 Sebastian Reimers
 */
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <re_atomic.h>
#include "vmix.h"

static struct vidpacket dummy_vp = {.timestamp = 0};
static mtx_t *vmix_mutex;


static inline void vmix_lock(void)
{
	mtx_lock(vmix_mutex);
}


static inline void vmix_unlock(void)
{
	mtx_unlock(vmix_mutex);
}


/* delegate main src thread */
static void vmix_delegate(void)
{
	struct vidsrc_st *st;

	vmix_lock();
	if (!vmix_srcl.head)
		goto out;

	st = vmix_srcl.head->data;
	if (!st)
		goto out;
	vmix_unlock();

	vidmix_source_start(st->vidmix_src);
	re_atomic_rlx_set(&st->run, true);

	return;
out:
	vmix_unlock();
}


static void destructor(void *arg)
{
	struct vidsrc_st *st = arg;
	bool delegate	     = vmix_srcl.head == &st->le;

	re_atomic_rlx_set(&st->run, false);

	if (st->vidisp)
		st->vidisp->vidsrc = NULL;

	list_unlink(&st->he);
	mem_deref(st->device);

	vidmix_source_enable(st->vidmix_src, false);
	vidmix_source_stop(st->vidmix_src);
	vmix_lock();
	list_unlink(&st->le);
	vmix_unlock();
	mem_deref(st->vidmix_src);

	if (delegate)
		vmix_delegate();
}


static void frame_handler(uint64_t ts, const struct vidframe *frame, void *arg)
{
	struct vidsrc_st *st = arg;
	struct le *le;

	if (!st || !st->frameh)
		return;

	if (!re_atomic_rlx(&st->run))
		return;

	st->frameh((struct vidframe *)frame, ts, st->arg);

	dummy_vp.keyframe = vmix_last_keyframe();

	vmix_lock();
	le = vmix_srcl.head;
	while (le) {
		st = le->data;
		if (!st)
			break;

		/* wait until keyframe arrive if src is not running */
		if (!re_atomic_rlx(&st->run) && !dummy_vp.keyframe) {
			le = le->next;
			continue;
		}

		re_atomic_rlx_set(&st->run, true);
		st->packeth(&dummy_vp, st->arg);
		le = le->next;
	}
	vmix_unlock();

	vmix_encode_flush();

	vmix_record(frame, ts);
}


int vmix_src_alloc(struct vidsrc_st **stp, const struct vidsrc *vs,
		   struct vidsrc_prm *prm, const struct vidsz *size,
		   const char *fmt, const char *dev, vidsrc_frame_h *frameh,
		   vidsrc_packet_h *packeth, vidsrc_error_h *errorh, void *arg)
{
	struct vidsrc_st *st;
	int err;
	(void)fmt;
	(void)errorh;
	(void)vs;

	if (!stp || !prm || !size || !frameh)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	st->packeth = packeth;
	st->frameh  = frameh;
	st->arg	    = arg;
	st->fps	    = prm->fps;

	err = str_dup(&st->device, dev);
	if (err)
		goto out;

	err = vidmix_source_alloc(&st->vidmix_src, vmix_mix, size, st->fps,
				  false, frame_handler, st);
	if (err)
		goto out;

	/* find a vidisp device with same name */
	st->vidisp = vmix_disp_find(dev);
	if (st->vidisp) {
		st->vidisp->vidsrc = st;
	}

	info("vidmix: src_alloc (%f fps)\n", st->fps);
	hash_append(vmix_src, hash_joaat_str(dev), &st->he, st);

	vmix_lock();
	list_append(&vmix_srcl, &st->le, st);
	vmix_unlock();

	vidmix_source_toggle_selfview(st->vidmix_src);

	vmix_request_keyframe();

	/* only start once */
	if (vmix_srcl.head == &st->le) {
		vidmix_source_start(st->vidmix_src);
		re_atomic_rlx_set(&st->run, true);
	}

out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static bool list_apply_handler(struct le *le, void *arg)
{
	struct vidsrc_st *st = le->data;

	return 0 == str_cmp(st->device, arg);
}


struct vidsrc_st *vmix_src_find(const char *device)
{
	return list_ledata(hash_lookup(vmix_src, hash_joaat_str(device),
				       list_apply_handler, (void *)device));
}


void vmix_src_input(struct vidsrc_st *st, const struct vidframe *frame,
		    uint64_t timestamp)
{
	(void)timestamp;

	if (!st || !frame)
		return;

	vidmix_source_put(st->vidmix_src, frame);
}


int vmix_src_init(void)
{
	return mutex_alloc(&vmix_mutex);
}


void vmix_src_close(void)
{
	vmix_mutex = mem_deref(vmix_mutex);
}
