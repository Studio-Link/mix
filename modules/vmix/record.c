/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2023 Sebastian Reimers
 */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include "libswresample/swresample.h"

#include <string.h>
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
	struct aubuf *ab;
	uint64_t video_start_time;
	uint64_t audio_start_time;
	char filename[512];
	AVFormatContext *outputFormatContext;
	AVStream *videoStream;
	AVStream *audioStream;
	AVCodecContext *videoCodecContext;
	AVCodecContext *audioCodecContext;
	SwrContext *resample_context;
} record = {0};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
	uint64_t ts;
	bool keyframe;
};


static int init_resampler(void)
{
	int ret;

	ret = swr_alloc_set_opts2(&record.resample_context,
				  &record.audioCodecContext->ch_layout,
				  record.audioCodecContext->sample_fmt,
				  record.audioCodecContext->sample_rate,
				  &(AVChannelLayout)AV_CHANNEL_LAYOUT_MONO,
				  AV_SAMPLE_FMT_S16, 48000, 0, NULL);
	if (ret < 0) {
		warning("Could not allocate resample context\n");
		return EINVAL;
	}

	ret = swr_init(record.resample_context);
	if (ret < 0) {
		warning("Could not init resample context\n");
		swr_free(&record.resample_context);
		return EINVAL;
	}

	return 0;
}


static int record_thread(void *arg)
{
	int ret;
	struct auframe af;
	struct le *le;
	AVRational timebase_video = {1, 1000000};
	int64_t audio_pts	  = 0;
	(void)arg;
	FILE *fp = fopen("test.pcm", "w");

	AVPacket *videoPacket = av_packet_alloc();
	AVPacket *audioPacket = av_packet_alloc();
	AVFrame *audioFrame   = av_frame_alloc();

	if (!videoPacket || !audioPacket || !audioFrame)
		return ENOMEM;

	audioFrame->nb_samples = record.audioCodecContext->frame_size;
	audioFrame->format     = record.audioCodecContext->sample_fmt;
	audioFrame->ch_layout  = record.audioCodecContext->ch_layout;

	ret = av_frame_get_buffer(audioFrame, 0);
	if (ret < 0) {
		warning("av_frame_get_buffer error %d\n", ret);
		return ENOMEM;
	}

	int16_t *sampv;
	sampv = mem_zalloc(
		record.audioCodecContext->frame_size * sizeof(int16_t), NULL);
	if (!sampv)
		return ENOMEM;

	auframe_init(&af, AUFMT_S16LE, sampv,
		     record.audioCodecContext->frame_size, 48000, 1);

	while (re_atomic_rlx(&record.run)) {
		sys_msleep(2);

		mtx_lock(record.lock);
		le = list_head(&record.bufs);
		mtx_unlock(record.lock);
		while (le) {
			struct record_entry *e = le->data;

			videoPacket->data	  = e->mb->buf;
			videoPacket->size	  = (int)e->size;
			videoPacket->stream_index = record.videoStream->index;

			videoPacket->dts = videoPacket->pts = av_rescale_q(
				e->ts - record.video_start_time,
				timebase_video, record.videoStream->time_base);
#if 0
			warning("ts: %llu, %lld %d/%d\n",
				e->ts - record.video_start_time,
				videoPacket->pts,
				record.videoStream->time_base.num,
				record.videoStream->time_base.den);
#endif

			ret = av_interleaved_write_frame(
				record.outputFormatContext, videoPacket);
			if (ret < 0) {
				warning("av_frame_write video error\n");
				return EINVAL;
			}

			mtx_lock(record.lock);
			le = le->next;
			mem_deref(e);
			mtx_unlock(record.lock);
		}

		while (aubuf_cur_size(record.ab) >= auframe_size(&af)) {
			audioFrame->nb_samples =
				record.audioCodecContext->frame_size;

			ret = av_frame_make_writable(audioFrame);
			if (ret < 0) {
				warning("av_frame_make_writable error\n");
				return ENOMEM;
			}

			aubuf_read_auframe(record.ab, &af);

			fwrite(af.sampv, 1, auframe_size(&af), fp);

			swr_convert(record.resample_context,
				    audioFrame->extended_data,
				    audioFrame->nb_samples,
				    (const uint8_t **)&af.sampv,
				    (int)af.sampc);

#if 0
			audioFrame->pts = av_rescale_q(
				af.timestamp -
					(record.video_start_time / 1000),
				timebase_audio, record.audioStream->time_base);
#else
			audioFrame->pts = audio_pts;
			audio_pts += audioFrame->nb_samples;
#endif

			ret = avcodec_send_frame(record.audioCodecContext,
						 audioFrame);
			if (ret < 0) {
				warning("Error sending the frame to the "
					"encoder\n");
				return ENOMEM;
			}

			while (ret >= 0) {
				int ret2;
				ret = avcodec_receive_packet(
					record.audioCodecContext, audioPacket);
				if (ret == AVERROR(EAGAIN) ||
				    ret == AVERROR_EOF)
					break;
				else if (ret < 0) {
					warning("Error encoding audio "
						"frame\n");
					return ENOMEM;
				}
#if 0
				warning("audio ts: %llu, %lld %lld %d/%d\n",
					af.timestamp -
						(record.video_start_time /
						 1000),
					audioFrame->pts, audioPacket->pts,
					record.audioStream->time_base.num,
					record.audioStream->time_base.den);
#endif
				audioPacket->stream_index =
					record.audioStream->index;

				ret2 = av_interleaved_write_frame(
					record.outputFormatContext,
					audioPacket);
				if (ret2 < 0) {
					warning("av_frame_write audio "
						"error\n");
					return EINVAL;
				}

				av_packet_unref(audioPacket);
			}
		}
	}

	fclose(fp);
	av_packet_free(&videoPacket);
	av_packet_free(&audioPacket);
	av_frame_free(&audioFrame);
	mem_deref(sampv);

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

	err = aubuf_alloc(&record.ab, 0, 0);
	if (err) {
		mem_deref(record.lock);
		return err;
	}

	aubuf_set_live(record.ab, false);

	ret = avformat_alloc_output_context2(&record.outputFormatContext, NULL,
					     NULL, record.filename);
	if (ret < 0) {
		warning("avformat_alloc error\n");
		return EINVAL;
	}

	/* VIDEO */
	const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	record.videoStream =
		avformat_new_stream(record.outputFormatContext, videoCodec);
	if (!record.videoStream)
		return ENOMEM;

	record.videoCodecContext = avcodec_alloc_context3(videoCodec);
	if (!record.videoCodecContext)
		return ENOMEM;

	record.videoCodecContext->width		= 1920;
	record.videoCodecContext->height	= 1080;
	record.videoCodecContext->time_base.num = 1;
	record.videoCodecContext->time_base.den = 30;
	record.videoCodecContext->pix_fmt	= AV_PIX_FMT_YUV420P;

	ret = avcodec_open2(record.videoCodecContext, videoCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 video error\n");
		return EINVAL;
	}
	avcodec_parameters_from_context(record.videoStream->codecpar,
					record.videoCodecContext);

	/* AUDIO */
	const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	record.audioStream =
		avformat_new_stream(record.outputFormatContext, audioCodec);
	if (!record.audioStream)
		return ENOMEM;

	record.audioCodecContext = avcodec_alloc_context3(audioCodec);
	if (!record.audioCodecContext)
		return ENOMEM;

	record.audioCodecContext->codec_id    = AV_CODEC_ID_AAC;
	record.audioCodecContext->codec_type  = AVMEDIA_TYPE_AUDIO;
	record.audioCodecContext->sample_rate = 48000;
	record.audioCodecContext->sample_fmt  = AV_SAMPLE_FMT_FLTP;
	record.audioCodecContext->ch_layout =
		(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	record.audioCodecContext->bit_rate = 96000;

	avcodec_open2(record.audioCodecContext, audioCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 audio error\n");
		return EINVAL;
	}
	avcodec_parameters_from_context(record.audioStream->codecpar,
					record.audioCodecContext);

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

	init_resampler();

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


void vmix_audio_record(struct auframe *af);
void vmix_audio_record(struct auframe *af)
{
	if (!re_atomic_rlx(&record.run) || !record.video_start_time)
		return;

	if (!record.audio_start_time)
		record.audio_start_time = af->timestamp;

	aubuf_write_auframe(record.ab, af);
}


int vmix_record(struct vidpacket *vp, RE_ATOMIC bool *update)
{
	struct record_entry *e;
	int err;
	(void)update;

	if (!re_atomic_rlx(&record.run))
		return ESHUTDOWN;

	if (!vp->buf || !vp->size)
		return EINVAL;

	if (!record.video_start_time) {
		/* wait until keyframe */
		if (!vp->keyframe) {
			/* FIXME: update request disabled for hardware enc */
			/* re_atomic_rlx_set(update, true); */
			return 0;
		}
		record.video_start_time = vp->timestamp;
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

	re_atomic_rlx_set(&record.run, false);
	thrd_join(record.thread, NULL);

	record.video_start_time = 0;
	record.audio_start_time = 0;

	/* Write the trailer and close the output file */
	av_write_trailer(record.outputFormatContext);

	/* Clean up */
	avcodec_close(record.videoCodecContext);
	avcodec_close(record.audioCodecContext);
	avformat_free_context(record.outputFormatContext);
	swr_free(&record.resample_context);

	list_flush(&record.bufs);

	record.ab = mem_deref(record.ab);

	mem_deref(record.lock);

	chmod(record.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	return 0;
}
