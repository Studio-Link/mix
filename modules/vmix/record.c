/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2023 Sebastian Reimers
 */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/rational.h>
#include <libswresample/swresample.h>

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
	RE_ATOMIC bool run_stream;
	struct list bufs;
	mtx_t *lock;
	thrd_t thread;
	struct aubuf *ab;
	RE_ATOMIC uint64_t video_start_time;
	RE_ATOMIC uint64_t audio_start_time;
	char filename[512];
	AVFormatContext *outputFormatContext;
	AVFormatContext *streamFormatContext;
	AVStream *videoStream;
	AVStream *videoStreamStream;
	AVStream *audioStream;
	AVStream *audioStreamStream;
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


static const char *stream_url	 = "rtmp://live.podstock.de/live/mux123";
static AVRational timebase_video = {1, 1000000};
static AVRational timebase_audio = {1, 48000};


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


static int init_stream(void)
{
	int ret;

	/* av_log_set_level(AV_LOG_DEBUG); */

	ret = avformat_alloc_output_context2(&record.streamFormatContext, NULL,
					     "flv", stream_url);
	if (ret < 0)
		return EINVAL;

	const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

	AVStream *videoStream =
		avformat_new_stream(record.streamFormatContext, videoCodec);
	if (!videoStream) {
		warning("video stream error\n");
		return ENOMEM;
	}

	AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoCodec);
	if (!videoCodecContext) {
		warning("videocontext error\n");
		return ENOMEM;
	}

	videoCodecContext->width	 = 1920;
	videoCodecContext->height	 = 1080;
	videoCodecContext->time_base.num = 1;
	videoCodecContext->time_base.den = 30;
	videoCodecContext->pix_fmt	 = AV_PIX_FMT_YUV420P;
	videoCodecContext->bit_rate	 = 4000000;

	if (record.streamFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ret = avcodec_open2(videoCodecContext, videoCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 video error\n");
		return EINVAL;
	}


	avcodec_parameters_from_context(videoStream->codecpar,
					videoCodecContext);

	const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);

	AVStream *audioStream =
		avformat_new_stream(record.streamFormatContext, audioCodec);
	if (!audioStream) {
		warning("audiostream error\n");
		return ENOMEM;
	}

	AVCodecContext *audioCodecContext = avcodec_alloc_context3(audioCodec);
	if (!audioCodecContext)
		return ENOMEM;

	audioCodecContext->codec_id    = AV_CODEC_ID_AAC;
	audioCodecContext->codec_type  = AVMEDIA_TYPE_AUDIO;
	audioCodecContext->sample_rate = 48000;
	audioCodecContext->sample_fmt  = AV_SAMPLE_FMT_FLTP;
	audioCodecContext->ch_layout =
		(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	audioCodecContext->bit_rate = 96000;
	if (record.streamFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	avcodec_open2(audioCodecContext, audioCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 audio error\n");
		return EINVAL;
	}

	avcodec_parameters_from_context(audioStream->codecpar,
					audioCodecContext);

	ret = avio_open(&record.streamFormatContext->pb, stream_url,
			AVIO_FLAG_WRITE);
	if (ret < 0) {
		warning("avio_open stream error\n");
		return EINVAL;
	}

	av_dump_format(record.streamFormatContext, 0, stream_url, 1);

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "flvflags", "no_duration_filesize", 0);

	ret = avformat_write_header(record.streamFormatContext, &opts);
	if (ret < 0) {
		warning("avformat_write_header stream error\n");
		return EINVAL;
	}

	re_atomic_rlx_set(&record.run_stream, true);

	record.videoStreamStream = videoStream;
	record.audioStreamStream = audioStream;

	return 0;
}


static void close_stream(void)
{
	if (re_atomic_rlx(&record.run_stream))
		av_write_trailer(record.streamFormatContext);

	if (record.streamFormatContext) {
		avio_close(record.streamFormatContext->pb);
		avformat_free_context(record.streamFormatContext);
	}

	re_atomic_rlx_set(&record.run_stream, false);
}


static int write_stream(AVPacket *pkt, AVRational *time_base_src,
			AVRational *time_base_dst)
{
	if (!re_atomic_rlx(&record.run_stream))
		return 0;

	AVPacket *packet = av_packet_clone(pkt);

	packet->pts =
		av_rescale_q(packet->pts, *time_base_src, *time_base_dst);

	packet->dts =
		av_rescale_q(packet->dts, *time_base_src, *time_base_dst);

	int ret =
		av_interleaved_write_frame(record.streamFormatContext, packet);
	if (ret < 0) {
		warning("av write stream error (%s)\n", av_err2str(ret));
		return EPIPE;
	}

	av_packet_free(&packet);

	return 0;
}


static int record_thread(void *arg)
{
	int ret;
	struct auframe af;
	struct le *le;
	int64_t audio_pts = 0;
	int err		  = 0;
	(void)arg;

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
			videoPacket->pos	  = -1;

			videoPacket->dts = videoPacket->pts = av_rescale_q(
				e->ts - re_atomic_rlx(
						&record.video_start_time),
				timebase_video, record.videoStream->time_base);
#if 0
			warning("ts: %llu, %lld %d/%d\n",
				e->ts - re_atomic_rlx(
						&record.video_start_time),
				videoPacket->pts,
				record.videoStream->time_base.num,
				record.videoStream->time_base.den);
#endif

			err = write_stream(
				videoPacket, &record.videoStream->time_base,
				&record.videoStreamStream->time_base);
			if (err)
				return err;

			ret = av_interleaved_write_frame(
				record.outputFormatContext, videoPacket);
			if (ret < 0) {
				warning("av_frame_write video stream error "
					"%s\n",
					av_err2str(ret));
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

			swr_convert(record.resample_context,
				    audioFrame->extended_data,
				    audioFrame->nb_samples,
				    (const uint8_t **)&af.sampv,
				    (int)af.sampc);

#if 0
			audioFrame->pts = av_rescale_q(
				af.timestamp -
					(re_atomic_rlx(
						 &record.video_start_time) /
					 1000),
				timebase_audio,
				record.audioStreamStream->time_base);
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
						(re_atomic_rlx(
						&record.video_start_time) /
						 1000),
					audioFrame->pts, audioPacket->pts,
					record.audioStreamStream->time_base
						.num,
					record.audioStreamStream->time_base
						.den);
#endif
				audioPacket->stream_index =
					record.audioStream->index;

				err = write_stream(
					audioPacket, &timebase_audio,
					&record.audioStreamStream->time_base);
				if (err)
					return err;

				ret2 = av_interleaved_write_frame(
					record.outputFormatContext,
					audioPacket);
				if (ret2 < 0) {
					warning("av_frame_write audio "
						"error\n");
					return EINVAL;
				}
			}
		}
	}

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

	/* Some formats want stream headers to be separate. */
	if (record.outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		record.videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
	/* Some formats want stream headers to be separate. */
	if (record.outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		record.audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
	init_stream();

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
	if (!re_atomic_rlx(&record.run) ||
	    !re_atomic_rlx(&record.video_start_time))
		return;

	if (!re_atomic_rlx(&record.audio_start_time))
		re_atomic_rlx_set(&record.audio_start_time, af->timestamp);

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

	if (!re_atomic_rlx(&record.video_start_time)) {
		/* wait until keyframe */
		if (!vp->keyframe) {
			/* FIXME: update request disabled for hardware enc */
			/* re_atomic_rlx_set(update, true); */
			return 0;
		}
		re_atomic_rlx_set(&record.video_start_time, vp->timestamp);
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

	re_atomic_rlx_set(&record.audio_start_time, 0);
	re_atomic_rlx_set(&record.video_start_time, 0);

	close_stream();

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
