/**
 * @file aumix/record.c aumix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "amix.h"


static struct {
	struct list tracks;
	RE_ATOMIC bool run;
	thrd_t thread;
	struct aubuf *ab;
	char *folder;
	uint64_t msecs;
} record = {.tracks = LIST_INIT, .run = false};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
};


uint64_t amix_record_msecs(void)
{
	if (!record.msecs)
		return 0;
	return tmr_jiffies() - record.msecs;
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
	int err;

	LIST_FOREACH(&record.tracks, le)
	{
		struct track *t = le->data;

		if (t->id == af->id) {
			track = t;
			break;
		}
	}

	if (!track) {
		track = mem_zalloc(sizeof(struct track), track_destruct);
		if (!track)
			return ENOMEM;

		track->id   = af->id;
		track->last = record.msecs;

		/* TODO: add user->name */
		re_snprintf(track->file, sizeof(track->file),
			    "%s/audio_id%u.flac", record.folder, track->id);

		err = flac_init(&track->flac, af, track->file);
		if (err) {
			mem_deref(track);
			return err;
		}

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
	int err;
	(void)arg;

	int16_t *sampv;
	size_t sampc = SRATE * CH * PTIME / 1000;

	sampv = mem_zalloc(sampc * sizeof(int16_t), NULL);
	if (!sampv)
		return ENOMEM;

	auframe_init(&af, AUFMT_S16LE, sampv, sampc, SRATE, CH);

	if (!record.msecs)
		record.msecs = tmr_jiffies();

	while (re_atomic_rlx(&record.run)) {
		sys_msleep(4);
		while (aubuf_cur_size(record.ab) > sampc) {
			aubuf_read_auframe(record.ab, &af);
			err = record_track(&af);
			if (err)
				goto out;
		}
	}

out:
	record.msecs = 0;
	mem_deref(sampv);

	return 0;
}


int amix_record_start(const char *folder)
{
	int err;

	if (!folder)
		return EINVAL;

	if (re_atomic_rlx(&record.run))
		return EALREADY;

	record.msecs = 0;
	str_dup(&record.folder, folder);

	err = aubuf_alloc(&record.ab, 0, 0);
	if (err) {
		return err;
	}

	re_atomic_rlx_set(&record.run, true);
	info("aumix: record started\n");

	thread_create_name(&record.thread, "aumix record", record_thread,
			   NULL);

	return 0;
}


void amix_record(struct auframe *af)
{
	if (!re_atomic_rlx(&record.run) || !af->id)
		return;

	af->timestamp = tmr_jiffies();
	aubuf_write_auframe(record.ab, af);
}


int amix_record_close(void)
{
	if (!re_atomic_rlx(&record.run))
		return EINVAL;

	re_atomic_rlx_set(&record.run, false);
	info("aumix: record close\n");
	thrd_join(record.thread, NULL);

	mem_deref(record.ab);

	record.folder = mem_deref(record.folder);
	list_flush(&record.tracks);

	return 0;
}
