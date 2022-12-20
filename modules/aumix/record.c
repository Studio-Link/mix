/**
 * @file aumix/record.c aumix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aumix.h"

static struct {
	struct list tracks;
	bool run;
	mtx_t *lock;
	thrd_t thread;
	struct aubuf *ab;
	char filename[512];
} record = {.tracks = LIST_INIT, .run = false};

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
	if (!record_msecs)
		return 0;
	return tmr_jiffies() - record_msecs;
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


struct track {
	struct le le;
	uint16_t id;
	char file[512];
	uint64_t last;
	struct flac *flac;
};


static void track_destruct(void *arg)
{
	struct track *track = arg;

	list_unlink(&track->le);
	mem_deref(track->flac);
}


static int record_track(struct auframe *af)
{
	struct le *le;
	struct track *track = NULL;
	uint64_t offset;

	LIST_FOREACH(&record.tracks, le)
	{
		track = le->data;

		if (track->id == af->id)
			break;
	}

	if (!track) {
		track = mem_zalloc(sizeof(struct track), track_destruct);
		if (!track)
			return ENOMEM;

		track->id   = af->id;
		track->last = record_msecs;

		/* TODO: add user->name */
		re_snprintf(record.filename, sizeof(record.filename),
			    "%s/audio_id%u.flac", record_folder, track->id);

		flac_init(&track->flac, af, record.filename);

		list_append(&record.tracks, &track->le, track);
	}

	offset	    = af->timestamp - track->last;
	track->last = af->timestamp;

	flac_record(track->flac, af, offset);

	return 0;
}


static int record_thread(void *arg)
{
	struct auframe af;
	(void)arg;

	int16_t *sampv;
	size_t sampc = SRATE * CH * PTIME / 1000;

	sampv = mem_zalloc(sampc * sizeof(int16_t), NULL);
	if (!sampv)
		return ENOMEM;

	auframe_init(&af, AUFMT_S16LE, sampv, sampc, SRATE, CH);

	if (!record_msecs)
		record_msecs = tmr_jiffies();

	while (record.run) {
		sys_msleep(4);
		while (aubuf_cur_size(record.ab) > sampc) {
			aubuf_read_auframe(record.ab, &af);
			record_track(&af);
		}
	}

	mem_deref(sampv);

	return 0;
}


int vidmix_record_start(char *record_folder);
int aumix_record_start(char *token)
{
	int err;

	if (record.run)
		return EALREADY;

	record_msecs = 0;

	mkdir_folder(token);

	err = mutex_alloc(&record.lock);
	if (err)
		return err;

	err = aubuf_alloc(&record.ab, 0, 0);
	if (err) {
		mem_deref(record.lock);
		return err;
	}

	record.run = true;
	info("aumix: record started\n");

	vidmix_record_start(record_folder);

	thread_create_name(&record.thread, "aumix record", record_thread,
			   NULL);

	return 0;
}


void aumix_record(struct auframe *af)
{
	if (!record.run || !af->id)
		return;

	af->timestamp = tmr_jiffies();
	aubuf_write_auframe(record.ab, af);
}


static int ffmpeg_final(void *arg)
{
	char *folder = arg;
	char *cmd;
	int err;

	if (!folder)
		return EINVAL;

	/* Audio/Video MP4 conversion */
	err = re_sdprintf(
		&cmd,
		"cd %s && ffmpeg -f s16le -ar %d -ac %d -i audio.pcm -i "
		"video.h264 -c:v copy -c:a aac record.mp4",
		folder, SRATE, CH);
	if (err)
		goto out;

	system(cmd);
	mem_deref(cmd);

	/* Audio FLAC conversion */
	err = re_sdprintf(&cmd,
			  "cd %s && ffmpeg -f s16le -ar %d -ac %d -i "
			  "audio.pcm audio.flac",
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
	thrd_join(record.thread, NULL);

	record_msecs = 0;
	list_flush(&record.tracks);

	mem_deref(record.ab);
	mem_deref(record.lock);

	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	str_dup(&folder, record_folder);

	/* re_thread_async(ffmpeg_final, NULL, folder); */
}
