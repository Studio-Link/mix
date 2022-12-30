/**
 * @file vidmix/src.c vidmix source
 *
 * Copyright (C) 2021-2022 Sebastian Reimers
 */
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "vidmix.h"

static struct vidpacket vp     = {.buf = NULL, .size = 0, .timestamp = 0};
static struct mbuf *packet_dup = NULL;
static bool reset	       = false;
static bool is_keyframe	       = false;

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
static void vidmix_delegate(void)
{
	struct vidsrc_st *st;

	vmix_lock();
	if (!vidmix_srcl.head)
		goto out;

	st = vidmix_srcl.head->data;
	if (!st)
		goto out;
	vmix_unlock();

	vidmix_source_start(st->vidmix_src);
	mtx_lock(st->lock);
	st->run = true;
	mtx_unlock(st->lock);

	return;
out:
	vmix_unlock();
}


static void destructor(void *arg)
{
	struct vidsrc_st *st = arg;
	bool delegate	     = vidmix_srcl.head == &st->le2;

	mtx_lock(st->lock);
	st->run = false;
	mtx_unlock(st->lock);

	if (st->vidisp)
		st->vidisp->vidsrc = NULL;

	vmix_lock();
	list_unlink(&st->le);
	vmix_unlock();

	mem_deref(st->device);
	vidmix_source_enable(st->vidmix_src, false);
	vidmix_source_stop(st->vidmix_src);
	list_unlink(&st->le2);
	mem_deref(st->vidmix_src);
	mem_deref(st->lock);

	if (delegate)
		vidmix_delegate();
}


static void frame_handler(uint64_t ts, const struct vidframe *frame, void *arg)
{
	struct vidsrc_st *st = arg;
	struct le *le;

	if (!st || !st->frameh)
		return;

	mtx_lock(st->lock);
	if (!st->run) {
		mtx_unlock(st->lock);
		return;
	}
	mtx_unlock(st->lock);

	/* frameh can return without calling dup_handler if not all network
	 * packets are send */
	st->frameh((struct vidframe *)frame, ts, st->arg);

	if (!vp.buf)
		return;

	if (!vidmix_srcl.head)
		return;

	vmix_lock();
	le = vidmix_srcl.head->next;
	vmix_unlock();
	while (le) {
		st = le->data;
		if (!st)
			break;

		/* wait until keyframe arrive if src is not running */
		if (!st->run && !is_keyframe) {
			le = le->next;
			continue;
		}

		st->run = true;
		st->packeth(&vp, st->arg);
		le = le->next;
	}

	/* record */
	vidmix_record(vp.buf, vp.size, &reset);

	/* prevent sending packets multiple times */
	vp.buf = NULL;
}


int packet_dup_handler(uint64_t ts, uint8_t *buf, size_t size, int keyframe);
int packet_dup_handler(uint64_t ts, uint8_t *buf, size_t size, int keyframe)
{
	int err = 0;

	if (!buf)
		return 0;

	if (reset) {
		reset = false;
		err   = ECONNRESET;
		goto out;
	}

	packet_dup->pos = 0;
	packet_dup->end = 0;

	err = mbuf_write_mem(packet_dup, buf, size);
	if (err) {
		warning("packet_dup_handler %m", err);
		return 0;
	}

	vp.timestamp = ts;
	vp.size	     = size;
	vp.buf	     = packet_dup->buf;
	is_keyframe  = keyframe;

out:
	return err;
}


int vidmix_src_alloc(struct vidsrc_st **stp, const struct vidsrc *vs,
		     struct vidsrc_prm *prm, const struct vidsz *size,
		     const char *fmt, const char *dev, vidsrc_frame_h *frameh,
		     vidsrc_packet_h *packeth, vidsrc_error_h *errorh,
		     void *arg)
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

	err = vidmix_source_alloc(&st->vidmix_src, vidmix_mix, size, st->fps,
				  false, frame_handler, st);
	if (err)
		goto out;

	err = mutex_alloc(&st->lock);
	if (err)
		goto out;

	/* find a vidisp device with same name */
	st->vidisp = vidmix_disp_find(dev);
	if (st->vidisp) {
		st->vidisp->vidsrc = st;
	}

	warning("vidmix: src_alloc (%f fps)\n", st->fps);
	hash_append(vidmix_src, hash_joaat_str(dev), &st->le, st);

	vmix_lock();
	list_append(&vidmix_srcl, &st->le2, st);
	vmix_unlock();

	vidmix_source_toggle_selfview(st->vidmix_src);

	reset = true;

	/* only start once */
	if (vidmix_srcl.head == &st->le2) {
		vidmix_source_start(st->vidmix_src);
		mtx_lock(st->lock);
		st->run = true;
		mtx_unlock(st->lock);
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


struct vidsrc_st *vidmix_src_find(const char *device)
{
	return list_ledata(hash_lookup(vidmix_src, hash_joaat_str(device),
				       list_apply_handler, (void *)device));
}


void vidmix_src_input(struct vidsrc_st *st, const struct vidframe *frame,
		      uint64_t timestamp)
{
	(void)timestamp;

	if (!st || !frame)
		return;

	vidmix_source_put(st->vidmix_src, frame);
}


int vidmix_src_init(void)
{
	packet_dup = mbuf_alloc(1024);
	if (!packet_dup) {
		return ENOMEM;
	}

	return mutex_alloc(&vmix_mutex);
}


void vidmix_src_close(void)
{
	vmix_mutex = mem_deref(vmix_mutex);
	packet_dup = mem_deref(packet_dup);
}
