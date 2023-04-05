/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <time.h>
#include <sys/stat.h>
#include <re_atomic.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <re_atomic.h>
#include "vmix.h"

static struct {
	FILE *f;
	RE_ATOMIC bool run;
	struct list bufs;
	mtx_t *lock;
	thrd_t thread;
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


static int record_thread(void *arg)
{
	size_t ret;
	struct le *le;
	(void)arg;

	while (re_atomic_rlx(&record.run)) {
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
				warning("vidmix/record: fwrite error\n");
			}
			mtx_lock(record.lock);
			le = le->next;
			mem_deref(e);
			mtx_unlock(record.lock);
		}
	}

	return 0;
}


int vmix_record_start(const char *record_folder)
{
	int err;

	if (re_atomic_rlx(&record.run))
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

	re_atomic_rlx_set(&record.run, true);
	info("vidmix: record started\n");

	thread_create_name(&record.thread, "vidrec", record_thread, NULL);

	return 0;
}


static void entry_destruct(void *arg)
{
	struct record_entry *e = arg;
	mem_deref(e->mb);

	list_unlink(&e->le);
}


int vmix_record(const uint8_t *buf, size_t size, RE_ATOMIC bool *update)
{
	struct record_entry *e;
	int err;

	if (!re_atomic_rlx(&record.run))
		return ESHUTDOWN;

	if (!buf || !size || !record.f)
		return EINVAL;

	if (!record.started) {
		re_atomic_rlx_set(update, true);
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


int vmix_record_close(void)
{
	if (!re_atomic_rlx(&record.run))
		return EINVAL;

	double avg_fps = .0;

	avg_fps = record.frames / ((tmr_jiffies() - record.start_time) * .001);

	re_atomic_rlx_set(&record.run, false);
	record.started = false;
	record.frames  = 0;
	info("vidmix: record close avg %.2f fps\n", avg_fps);
	thrd_join(record.thread, NULL);

	/* FIXME: maybe not all frames are written */
	list_flush(&record.bufs);

	mem_deref(record.lock);

	fclose(record.f);
	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	record.f = NULL;

	return 0;
}


static int vmix_record_create(const char *dest)
{
	AVFormatContext *ctx;
	AVStream *stream;
	AVCodecContext *enc;
	AVDictionary *opts = NULL;
	int fps		   = 25;
	int err, ret;

	if (!str_isset(dest))
		return EINVAL;

	ctx = avformat_alloc_context();
	if (!ctx) {
		warning("vmix_rec: avformat alloc error\n");
		return ENOMEM;
	}

	ctx->oformat = av_guess_format("mp4", NULL, NULL);
	if (!ctx->oformat) {
		warning("vmix_rec: oformat not available\n");
		return EINVAL;
	}

	ctx->video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!ctx->video_codec) {
		warning("vmix_rec: encoder not available\n");
		return EINVAL;
	}

	stream = avformat_new_stream(ctx, ctx->video_codec);
	if (!stream) {
		warning("vmix_rec: new stream error\n");
		return ENOMEM;
	}

	stream->id = ctx->nb_streams - 1;

	enc = avcodec_alloc_context3(ctx->video_codec);
	if (!enc) {
		warning("vmix_rec: new enc context error\n");
		return ENOMEM;
	}

	enc->width     = 1920;
	enc->height    = 1080;
	enc->time_base = (AVRational){1, fps};
	enc->pix_fmt   = AV_PIX_FMT_YUV420P;
	enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_open2(enc, ctx->video_codec, NULL) < 0) {
		warning("vmix_rec: avcodec_open2 error\n");
		return EINVAL;
	}

	avcodec_parameters_from_context(stream->codecpar, enc);
	av_dict_set(&opts, "movflags", "+faststart", 0);

	ret = avio_open2(&ctx->pb, dest, AVIO_FLAG_WRITE, NULL, &opts);
	if (ret < 0) {
		warning("vmix_rec: avio_open2 error %s\n", av_err2str(ret));
		return EINVAL;
	}

	err = str_dup(&ctx->url, dest);
	if (err)
		return err;

	ret = avformat_write_header(ctx, &opts);
	if (ret < 0) {
		warning("vmix_rec: avformat_write_header error %s\n",
			av_err2str(ret));
		return EINVAL;
	}

	return 0;
}
