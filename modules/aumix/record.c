/**
 * @file aumix/record.c aumix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aumix.h"

static struct {
	FILE *f;
	bool run;
	struct list bufs;
	mtx_t *lock;
	pthread_t thread;
	char filename[512];
} record = {NULL, false, LIST_INIT, NULL, 0, {0}};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
};

static char record_folder[256] = {0};
static uint64_t record_msecs   = 0;


uint64_t aumix_record_msecs(void);
uint64_t aumix_record_msecs(void)
{
	return record_msecs;
}


static int timestamp_print(struct re_printf *pf, const struct tm *tm)
{
	if (!tm)
		return 0;

	return re_hprintf(pf, "%d-%02d-%02d-%02d-%02d-%02d",
			  1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			  tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static void mkdir_folder(char *token)
{
	time_t tnow   = time(0);
	struct tm *tm = localtime(&tnow);

	(void)re_snprintf(record_folder, sizeof(record_folder),
			  "webui/public/download/%s", token);

	fs_mkdir(record_folder, 0755);

	(void)re_snprintf(record_folder, sizeof(record_folder), "%s/%H",
			  record_folder, timestamp_print, tm);

	fs_mkdir(record_folder, 0755);
}


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

			ret = fwrite(e->mb->buf, e->size, 1, record.f);
			if (!ret) {
				warning("aumix/record: fwrite error\n");
			}

			mtx_lock(record.lock);
			record_msecs += PTIME;
			le = le->next;
			mem_deref(e);
			mtx_unlock(record.lock);
		}
	}

	return NULL;
}


int vidmix_record_start(char *record_folder);
int aumix_record_start(char *token)
{
	int err;

	if (record.run)
		return EALREADY;

	record_msecs = 0;

	mkdir_folder(token);
	re_snprintf(record.filename, sizeof(record.filename), "%s/audio.pcm",
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
	info("aumix: record started\n");

	vidmix_record_start(record_folder);

	pthread_create(&record.thread, NULL, record_thread, NULL);

	return 0;
}


static void entry_destruct(void *arg)
{
	struct record_entry *e = arg;
	mem_deref(e->mb);
	list_unlink(&e->le);
}


int aumix_record(const uint8_t *buf, size_t size)
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


static int ffmpeg_final(void *arg)
{
	char *folder = arg;
	char *cmd;
	int err;

	if (!folder)
		return EINVAL;

	err = re_sdprintf(
		&cmd,
		"cd %s && ffmpeg -f s16le -ar %d -ac %d -i audio.pcm -i "
		"video.h264 -c:v copy -c:a aac record.mp4",
		folder, SRATE, CH);
	if (err)
		goto out;

	system(cmd);

	mem_deref(cmd);

out:
	mem_deref(folder);

	return err;
}


void vidmix_record_close(void);
void aumix_record_close(void)
{
	if (!record.run)
		return;
	char *folder = NULL;

	vidmix_record_close();

	record.run = false;
	info("aumix: record close\n");
	pthread_join(record.thread, NULL);

	record_msecs = 0;

	list_flush(&record.bufs);

	mem_deref(record.lock);

	fclose(record.f);
	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	record.f = NULL;

	str_dup(&folder, record_folder);

	re_thread_async(ffmpeg_final, NULL, folder);
}
