/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <time.h>
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "vidmix.h"

static struct {
	FILE *f;
	bool run;
	struct list bufs;
	mtx_t *lock;
	pthread_t thread;
} record = {
	NULL,
	false,
	LIST_INIT,
	NULL,
	0
};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
};

static char record_folder[128] = {0};

static int timestamp_print(struct re_printf *pf, const struct tm *tm)
{
	if (!tm)
		return 0;

	return re_hprintf(pf, "%d-%02d-%02d-%02d-%02d-%02d",
			1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static void mkdir_folder(void)
{
	char mqttclientid[64] = {0};
	time_t tnow = time(0);
	struct tm *tm = localtime(&tnow);

	conf_get_str(conf_cur(), "mqtt_broker_clientid",
		     mqttclientid, sizeof(mqttclientid));

	(void)re_snprintf(record_folder, sizeof(record_folder),
		"/tmp/%s", mqttclientid);
	
	fs_mkdir(record_folder, 0700);

	(void)re_snprintf(record_folder, sizeof(record_folder),
		"%s/%H", record_folder, timestamp_print, tm);

	fs_mkdir(record_folder, 0700);
}


static void *record_thread(void *arg)
{
	size_t ret;
	struct le *le;
	(void)arg;

	while (record.run)
	{
		sys_msleep(4);

		if (!record.f)
			continue;
		
		mtx_lock(record.lock);
		le = list_head(&record.bufs);
		mtx_unlock(record.lock);
		while(le) {
			struct record_entry *e = le->data;
			if (!e) {
				le = le->next; 
				continue;
			}
			ret = fwrite(e->mb->buf, e->size, 1, record.f);
			if (!ret) {
				warning("vidmix/record: fwrite error\n");
			}
			le = le->next; 
			mem_deref(e);
		}
	}

	return NULL;
}


int vidmix_record_start(void)
{
	int err;
	char filename[256] = {0};

	if (record.run)
		return EALREADY;

	mkdir_folder();
	re_snprintf(filename, sizeof(filename),
			"%s/video.h264", record_folder);

	err = fs_fopen(&record.f, filename, "w+");
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
	
	mtx_lock(record.lock);
	list_unlink(&e->le);
	mtx_unlock(record.lock);
}


int vidmix_record(const uint8_t *buf, size_t size)
{
	struct record_entry *e;
	int err;
	
	if (!buf || !size || !record.f)
		return EINVAL;

	if (!record.run)
		return ESHUTDOWN;

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
	mtx_unlock(record.lock);

out:
	if (err)
		mem_deref(e);

	return err;
}


void vidmix_record_close(void)
{
	if (!record.run)
		return;

	record.run = false;
	info("vidmix: record close\n");
	pthread_join(record.thread, NULL);

	/* FIXME: maybe not all frames are written */
	list_flush(&record.bufs);

	mem_deref(record.lock);

	fclose(record.f);
	record.f = NULL;
}
