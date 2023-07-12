/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2022 Sebastian Reimers
 */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>

#include <time.h>
#include <sys/stat.h>
#include <re_atomic.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <re_atomic.h>
#include "vmix.h"

static struct {
	RE_ATOMIC bool run;
	struct list bufs;
	mtx_t *lock;
	thrd_t thread;
	uint64_t frames;
	uint64_t start_time;
	char filename[512];
	AVFormatContext *outputFormatContext;
	AVStream *videoStream;
	AVCodecContext *videoCodecContext;
} record = {false, LIST_INIT, NULL, 0, 0, 0, {0}, NULL, NULL, NULL};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
	uint64_t ts;
	bool keyframe;
};


static int record_thread(void *arg)
{
	size_t ret;
	struct le *le;
	AVRational timebase = {1, 1000000};
	(void)arg;

	AVPacket *videoPacket = av_packet_alloc();
	if (!videoPacket)
		return ENOMEM;

	videoPacket->stream_index = record.videoStream->index;

	while (re_atomic_rlx(&record.run)) {
		sys_msleep(2);

		mtx_lock(record.lock);
		le = list_head(&record.bufs);
		mtx_unlock(record.lock);
		while (le) {
			struct record_entry *e = le->data;

			videoPacket->data = e->mb->buf;
			videoPacket->size = (int)e->size;

			videoPacket->dts = videoPacket->pts = av_rescale_q(
				e->ts - record.start_time, timebase,
				record.outputFormatContext->streams[0]
					->time_base);
#if 0
			warning("ts: %llu, %lld %d/%d\n",
				e->ts - record.start_time, videoPacket->pts,
				record.outputFormatContext->streams[0]
					->time_base.num,
				record.outputFormatContext->streams[0]
					->time_base.den);
#endif
			ret = av_write_frame(record.outputFormatContext,
					     videoPacket);

			if (ret < 0) {
				warning("av_frame_write error\n");
				return EINVAL;
			}

			record.frames++;

			mtx_lock(record.lock);
			le = le->next;
			mem_deref(e);
			mtx_unlock(record.lock);
		}
	}

	av_packet_free(&videoPacket);

	return 0;
}


int vmix_record_start(const char *record_folder)
{
	int err;
	int ret;

	if (re_atomic_rlx(&record.run))
		return EALREADY;

	re_snprintf(record.filename, sizeof(record.filename), "%s/video.mp4",
		    record_folder);

	err = mutex_alloc(&record.lock);
	if (err) {
		return err;
	}

	ret = avformat_alloc_output_context2(&record.outputFormatContext, NULL,
					     NULL, record.filename);
	if (ret < 0) {
		warning("avformat_alloc error\n");
		return EINVAL;
	}

	const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	record.videoStream =
		avformat_new_stream(record.outputFormatContext, videoCodec);
	if (!record.videoStream)
		return ENOMEM;

	record.videoCodecContext	 = avcodec_alloc_context3(videoCodec);
	record.videoCodecContext->width	 = 1920;
	record.videoCodecContext->height = 1080;
	record.videoCodecContext->time_base.num = 1;
	record.videoCodecContext->time_base.den = 30;
	record.videoCodecContext->pix_fmt	= AV_PIX_FMT_YUV420P;

	ret = avcodec_open2(record.videoCodecContext, videoCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 error\n");
		return EINVAL;
	}
	avcodec_parameters_from_context(record.videoStream->codecpar,
					record.videoCodecContext);

	ret = avio_open(&record.outputFormatContext->pb, record.filename,
			AVIO_FLAG_WRITE);
	if (ret < 0) {
		warning("avio_open error\n");
		return EINVAL;
	}

	ret = avformat_write_header(record.outputFormatContext, NULL);
	if (ret < 0) {
		warning("avformat_write_header error\n");
		return EINVAL;
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


int vmix_record(struct vidpacket *vp, RE_ATOMIC bool *update)
{
	struct record_entry *e;
	int err;

	if (!re_atomic_rlx(&record.run))
		return ESHUTDOWN;

	if (!vp->buf || !vp->size)
		return EINVAL;

	if (!record.start_time) {
		/* wait until keyframe */
		if (!vp->keyframe) {
			re_atomic_rlx_set(update, true);
			return 0;
		}
		record.start_time = vp->timestamp;
	}

	e = mem_zalloc(sizeof(struct record_entry), entry_destruct);
	if (!e)
		return ENOMEM;

	e->mb = mbuf_alloc(vp->size);
	if (!e->mb) {
		err = ENOMEM;
		goto out;
	}

	err = mbuf_write_mem(e->mb, vp->buf, vp->size);
	if (err)
		goto out;

	e->size	    = vp->size;
	e->ts	    = vp->timestamp;
	e->keyframe = vp->keyframe;

	mtx_lock(record.lock);
	list_append(&record.bufs, &e->le, e);
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
	record.start_time = 0;
	record.frames  = 0;
	info("vidmix: record close avg %.2f fps\n", avg_fps);
	thrd_join(record.thread, NULL);

	// Write the trailer and close the output file
	av_write_trailer(record.outputFormatContext);

	// Clean up
	avcodec_close(record.videoCodecContext);
	avformat_free_context(record.outputFormatContext);

	/* FIXME: maybe not all frames are written */
	list_flush(&record.bufs);

	mem_deref(record.lock);

	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	return 0;
}
