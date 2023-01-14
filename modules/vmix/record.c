/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "vmix.h"

static struct {
	FILE *f;
	bool run;
	struct list bufs;
	mtx_t *lock;
	pthread_t thread;
	bool started;
	uint64_t frames;
	uint64_t start_time;
	char filename[512];
} record = {NULL, false, LIST_INIT, NULL, 0, false, 0, 0, {0}};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
};


static void *record_thread(void *arg)
{
	size_t ret;
	struct le *le;
	(void)arg;

	while (record.run) {
		sys_msleep(4);

		if (!record.f)
			continue;

		mtx_lock(record.lock);
		le = list_head(&record.bufs);
		mtx_unlock(record.lock);
		while (le) {
			struct record_entry *e = le->data;
			if (!e) {
				le = le->next;
				continue;
			}
			ret = fwrite(e->mb->buf, e->size, 1, record.f);
			if (!ret) {
				warning("vidmix/record: fwrite error\n");
			}
			mtx_lock(record.lock);
			le = le->next;
			mem_deref(e);
			mtx_unlock(record.lock);
		}
	}

	return NULL;
}


int vmix_record_start(char *record_folder)
{
	int err;

	if (record.run)
		return EALREADY;

	re_snprintf(record.filename, sizeof(record.filename), "%s/video.h264",
		    record_folder);

	err = fs_fopen(&record.f, record.filename, "w+");
	if (err)
		return err;

	err = mutex_alloc(&record.lock);
	if (err) {
		fclose(record.f);
		return err;
	}

	record.run = true;
	info("vidmix: record started\n");

	pthread_create(&record.thread, NULL, record_thread, NULL);

	return 0;
}


static void entry_destruct(void *arg)
{
	struct record_entry *e = arg;
	mem_deref(e->mb);

	list_unlink(&e->le);
}


int vmix_record(const uint8_t *buf, size_t size, bool *update)
{
	struct record_entry *e;
	int err;

	if (!buf || !size || !record.f)
		return EINVAL;

	if (!record.run)
		return ESHUTDOWN;

	if (!record.started) {
		*update		  = true;
		record.started	  = true;
		record.start_time = tmr_jiffies();
		return 0;
	}

	e = mem_zalloc(sizeof(struct record_entry), entry_destruct);
	if (!e)
		return ENOMEM;

	e->mb = mbuf_alloc(size);
	if (!e->mb) {
		err = ENOMEM;
		goto out;
	}

	err = mbuf_write_mem(e->mb, buf, size);
	if (err)
		goto out;

	e->size = size;

	mtx_lock(record.lock);
	list_append(&record.bufs, &e->le, e);
	record.frames++;
	mtx_unlock(record.lock);

out:
	if (err)
		mem_deref(e);

	return err;
}


void vmix_record_close(void)
{
	if (!record.run)
		return;

	double avg_fps = .0;

	avg_fps = record.frames / ((tmr_jiffies() - record.start_time) * .001);

	record.run     = false;
	record.started = false;
	record.frames  = 0;
	info("vidmix: record close avg %.2f fps\n", avg_fps);
	pthread_join(record.thread, NULL);

	/* FIXME: maybe not all frames are written */
	list_flush(&record.bufs);

	mem_deref(record.lock);

	fclose(record.f);
	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	record.f = NULL;
}
