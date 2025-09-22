#include <mix.h>
#include "re_thread.h"
#include "vmix.h"

static struct {
	struct list srcl;
	thrd_t thrd;
	RE_ATOMIC bool run;
	struct vidsrc *vidsrc;
	mtx_t *mtx;
} pktsrc;


static void pktsrc_deref(void *arg)
{
	struct vidsrc_st *st = arg;
	mtx_lock(pktsrc.mtx);
	mem_deref(st->device);
	list_unlink(&st->le);
	mtx_unlock(pktsrc.mtx);
}


static int pktsrc_alloc(struct vidsrc_st **stp, const struct vidsrc *vs,
			struct vidsrc_prm *prm, const struct vidsz *size,
			const char *fmt, const char *dev,
			vidsrc_frame_h *frameh, vidsrc_packet_h *packeth,
			vidsrc_error_h *errorh, void *arg)
{
	struct vidsrc_st *st;
	int err;
	(void)fmt;
	(void)errorh;
	(void)vs;

	if (!stp || !prm || !size || !frameh)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), pktsrc_deref);
	if (!st)
		return ENOMEM;

	st->packeth = packeth;
	st->frameh  = frameh;
	st->arg	    = arg;
	st->fps	    = prm->fps;

	err = str_dup(&st->device, dev);
	if (err)
		goto out;

	mtx_lock(pktsrc.mtx);
	list_append(&pktsrc.srcl, &st->le, st);
	mtx_unlock(pktsrc.mtx);

out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static int pktsrc_thread(void *arg)
{
	struct le *le;
	(void)arg;

	while (re_atomic_rlx(&pktsrc.run)) {
		sys_msleep(2);

		mtx_lock(pktsrc.mtx);
		LIST_FOREACH(&pktsrc.srcl, le)
		{
			struct vidsrc_st *st = le->data;

			vmix_codec_pkt(st);
		}
		mtx_unlock(pktsrc.mtx);
	}

	return 0;
}


int vmix_pktsrc_init(void)
{
	int err;

	err = mutex_alloc(&pktsrc.mtx);
	if (err)
		return err;

	err = vidsrc_register(&pktsrc.vidsrc, baresip_vidsrcl(), "vmix_pktsrc",
			      pktsrc_alloc, NULL);
	if (err)
		return err;

	re_atomic_rlx_set(&pktsrc.run, true);

	return thread_create_name(&pktsrc.thrd, "vmix_pktsrc", pktsrc_thread,
				  NULL);
}


void vmix_pktsrc_close(void)
{
	re_atomic_rlx_set(&pktsrc.run, false);
	thrd_join(pktsrc.thrd, NULL);

	pktsrc.vidsrc = mem_deref(pktsrc.vidsrc);
	pktsrc.mtx    = mem_deref(pktsrc.mtx);
}
