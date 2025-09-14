/**
 * @file vidmix/record.c vidmix recording
 *
 * Copyright (C) 2023-2024 Sebastian Reimers
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

#define STREAM 0

static struct {
	RE_ATOMIC bool run;
	RE_ATOMIC bool run_stream;
	struct list vframes;
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
	AVFrame *videoFrame;
	AVCodecContext *videoCodecCtx;
	AVCodecContext *audioCodecCtx;
	SwrContext *resample_context;
} rec = {.run = false};

struct record_entry {
	struct le le;
	uint64_t ts;
	struct vidframe *frame;
};

#if STREAM
static const char *stream_url	 = "rtmp://example.net/stream";
static AVRational timebase_audio = {1, 48000};
#endif
static AVRational timebase_video = {1, 1000000};


static int init_resampler(void)
{
	int ret;

	ret = swr_alloc_set_opts2(
		&rec.resample_context, &rec.audioCodecCtx->ch_layout,
		rec.audioCodecCtx->sample_fmt, rec.audioCodecCtx->sample_rate,
		&(AVChannelLayout)AV_CHANNEL_LAYOUT_MONO, AV_SAMPLE_FMT_S16,
		48000, 0, NULL);
	if (ret < 0) {
		warning("Could not allocate resample context\n");
		return EINVAL;
	}

	ret = swr_init(rec.resample_context);
	if (ret < 0) {
		warning("Could not init resample context\n");
		swr_free(&rec.resample_context);
		return EINVAL;
	}

	return 0;
}

#if STREAM
static int init_stream(void)
{
	int ret;

	/* av_log_set_level(AV_LOG_DEBUG); */

	ret = avformat_alloc_output_context2(&rec.streamFormatContext, NULL,
					     "flv", stream_url);
	if (ret < 0)
		return EINVAL;

	const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

	AVStream *videoStream =
		avformat_new_stream(rec.streamFormatContext, videoCodec);
	if (!videoStream) {
		warning("video stream error\n");
		return ENOMEM;
	}

	avcodec_parameters_from_context(videoStream->codecpar,
					rec.videoCodecCtx);

	const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);

	AVStream *audioStream =
		avformat_new_stream(rec.streamFormatContext, audioCodec);
	if (!audioStream) {
		warning("audiostream error\n");
		return ENOMEM;
	}

	avcodec_parameters_from_context(audioStream->codecpar,
					rec.audioCodecCtx);

	ret = avio_open(&rec.streamFormatContext->pb, stream_url,
			AVIO_FLAG_WRITE);
	if (ret < 0) {
		warning("avio_open stream error\n");
		return EINVAL;
	}

	av_dump_format(rec.streamFormatContext, 0, stream_url, 1);

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "flvflags", "no_duration_filesize", 0);

	ret = avformat_write_header(rec.streamFormatContext, &opts);
	if (ret < 0) {
		warning("avformat_write_header stream error\n");
		return EINVAL;
	}

	re_atomic_rlx_set(&rec.run_stream, true);

	rec.videoStreamStream = videoStream;
	rec.audioStreamStream = audioStream;

	return 0;
}


static void close_stream(void)
{
	if (re_atomic_rlx(&rec.run_stream))
		av_write_trailer(rec.streamFormatContext);

	if (rec.streamFormatContext) {
		avio_close(rec.streamFormatContext->pb);
		avformat_free_context(rec.streamFormatContext);
	}

	re_atomic_rlx_set(&rec.run_stream, false);
}


static int write_stream(AVPacket *pkt, AVRational *time_base_src,
			AVRational *time_base_dst)
{
	if (!re_atomic_rlx(&rec.run_stream))
		return 0;

	int err = 0;
	AVPacket *packet = av_packet_clone(pkt);

	packet->pts =
		av_rescale_q(packet->pts, *time_base_src, *time_base_dst);

	packet->dts =
		av_rescale_q(packet->dts, *time_base_src, *time_base_dst);

	int ret = av_interleaved_write_frame(rec.streamFormatContext, packet);
	if (ret < 0) {
		warning("av write stream error (%s)\n", av_err2str(ret));
		err = EPIPE;
	}

	av_packet_free(&packet);

	return err;
}
#endif


static int record_thread(void *arg)
{
	int ret;
	struct auframe af;
	struct le *le;
	int64_t audio_pts = 0;
#if STREAM
	int err = 0;
#endif
	(void)arg;

	AVPacket *videoPacket = av_packet_alloc();
	AVPacket *audioPacket = av_packet_alloc();
	AVFrame *audioFrame   = av_frame_alloc();

	if (!videoPacket || !audioPacket || !audioFrame)
		return ENOMEM;

	audioFrame->nb_samples = rec.audioCodecCtx->frame_size;
	audioFrame->format     = rec.audioCodecCtx->sample_fmt;
	audioFrame->ch_layout  = rec.audioCodecCtx->ch_layout;

	ret = av_frame_get_buffer(audioFrame, 0);
	if (ret < 0) {
		warning("av_frame_get_buffer error %d\n", ret);
		return ENOMEM;
	}

	int16_t *sampv;
	sampv = mem_zalloc(rec.audioCodecCtx->frame_size * sizeof(int16_t),
			   NULL);
	if (!sampv)
		return ENOMEM;

	auframe_init(&af, AUFMT_S16LE, sampv, rec.audioCodecCtx->frame_size,
		     48000, 1);

	while (re_atomic_rlx(&rec.run)) {
		sys_usleep(2000);

		mtx_lock(rec.lock);
		uint32_t count = list_count(&rec.vframes);
		if (count > 10)
			warning("list count %u\n", count);
		le = list_head(&rec.vframes);
		mtx_unlock(rec.lock);

		while (le) {
			struct record_entry *e = le->data;

			for (int i = 0; i < 4; i++) {
				rec.videoFrame->data[i] = e->frame->data[i];
				rec.videoFrame->linesize[i] =
					e->frame->linesize[i];
			}

			rec.videoFrame->pts = av_rescale_q(
				e->ts - re_atomic_rlx(&rec.video_start_time),
				timebase_video, rec.videoStream->time_base);

			ret = avcodec_send_frame(rec.videoCodecCtx,
						 rec.videoFrame);
			if (ret < 0) {
				warning("rec: Error sending the video frame "
					"to the encoder\n");
				return ENOMEM;
			}

			while (ret >= 0) {
				ret = avcodec_receive_packet(rec.videoCodecCtx,
							     videoPacket);
				if (ret == AVERROR(EAGAIN) ||
				    ret == AVERROR_EOF)
					break;
				else if (ret < 0) {
					warning("Error encoding video "
						"frame\n");
					return ENOMEM;
				}

				videoPacket->stream_index =
					rec.videoStream->index;
#if 0
				warning("ts: %llu, %lld %d/%d\n",
					e->ts - re_atomic_rlx(
							&rec.video_start_time),
					videoPacket->pts,
					rec.videoStream->time_base.num,
					rec.videoStream->time_base.den);
#endif

#if STREAM
				err = write_stream(
					videoPacket,
					&rec.videoStream->time_base,
					&rec.videoStreamStream->time_base);
				if (err)
					return err;
#endif

				int ret2 = av_interleaved_write_frame(
					rec.outputFormatContext, videoPacket);
				if (ret2 < 0) {
					warning("av_frame_write video stream "
						"error "
						"%s\n",
						av_err2str(ret));
					return EINVAL;
				}
			}

			mtx_lock(rec.lock);
			le = le->next;
			mtx_unlock(rec.lock);
			mem_deref(e);
		}

		while (aubuf_cur_size(rec.ab) >= auframe_size(&af)) {
			audioFrame->nb_samples = rec.audioCodecCtx->frame_size;

			ret = av_frame_make_writable(audioFrame);
			if (ret < 0) {
				warning("av_frame_make_writable error\n");
				return ENOMEM;
			}

			aubuf_read_auframe(rec.ab, &af);

			swr_convert(rec.resample_context,
				    audioFrame->extended_data,
				    audioFrame->nb_samples,
				    (const uint8_t **)&af.sampv,
				    (int)af.sampc);
#if 0
			audioFrame->pts = av_rescale_q(
				af.timestamp -
					(re_atomic_rlx(
						 &rec.video_start_time) /
					 1000),
				timebase_audio,
				rec.audioStreamStream->time_base);
#else
			audioFrame->pts = audio_pts;
			audio_pts += audioFrame->nb_samples;
#endif

			ret = avcodec_send_frame(rec.audioCodecCtx,
						 audioFrame);
			if (ret < 0) {
				warning("Error sending the frame to the "
					"encoder\n");
				return ENOMEM;
			}

			while (ret >= 0) {
				ret = avcodec_receive_packet(rec.audioCodecCtx,
							     audioPacket);
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
						&rec.video_start_time) /
						 1000),
					audioFrame->pts, audioPacket->pts,
					rec.audioStreamStream->time_base
						.num,
					rec.audioStreamStream->time_base
						.den);
#endif
				audioPacket->stream_index =
					rec.audioStream->index;
#if STREAM
				err = write_stream(
					audioPacket, &timebase_audio,
					&rec.audioStreamStream->time_base);
				if (err)
					return err;
#endif

				int ret2 = av_interleaved_write_frame(
					rec.outputFormatContext, audioPacket);
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

	if (re_atomic_rlx(&rec.run))
		return EALREADY;

	re_snprintf(rec.filename, sizeof(rec.filename), "%s/video.mp4",
		    record_folder);

	err = mutex_alloc(&rec.lock);
	if (err) {
		return err;
	}

	err = aubuf_alloc(&rec.ab, 0, 0);
	if (err) {
		mem_deref(rec.lock);
		return err;
	}

	aubuf_set_live(rec.ab, false);

	ret = avformat_alloc_output_context2(&rec.outputFormatContext, NULL,
					     NULL, rec.filename);
	if (ret < 0) {
		warning("avformat_alloc error\n");
		return EINVAL;
	}

	/* VIDEO */
	const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	rec.videoStream =
		avformat_new_stream(rec.outputFormatContext, videoCodec);
	if (!rec.videoStream)
		return ENOMEM;

	rec.videoCodecCtx = avcodec_alloc_context3(videoCodec);
	if (!rec.videoCodecCtx)
		return ENOMEM;

	struct config *conf = conf_config();

	rec.videoCodecCtx->width	 = conf->video.width;
	rec.videoCodecCtx->height	 = conf->video.height;
	rec.videoCodecCtx->time_base.num = 1;
	rec.videoCodecCtx->time_base.den = conf->video.fps;
	rec.videoCodecCtx->pix_fmt	 = AV_PIX_FMT_YUV420P;
	rec.videoCodecCtx->bit_rate	 = 8000000;
	rec.videoCodecCtx->thread_count	 = 1;

	av_opt_set(rec.videoCodecCtx->priv_data, "profile", "baseline", 0);
	av_opt_set(rec.videoCodecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(rec.videoCodecCtx->priv_data, "tune", "zerolatency", 0);

	/* Some formats want stream headers to be separate. */
	if (rec.outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		rec.videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ret = avcodec_open2(rec.videoCodecCtx, videoCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 video error\n");
		return EINVAL;
	}
	avcodec_parameters_from_context(rec.videoStream->codecpar,
					rec.videoCodecCtx);

	/* AUDIO */
	const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	rec.audioStream =
		avformat_new_stream(rec.outputFormatContext, audioCodec);
	if (!rec.audioStream)
		return ENOMEM;

	rec.audioCodecCtx = avcodec_alloc_context3(audioCodec);
	if (!rec.audioCodecCtx)
		return ENOMEM;

	rec.audioCodecCtx->codec_id    = AV_CODEC_ID_AAC;
	rec.audioCodecCtx->codec_type  = AVMEDIA_TYPE_AUDIO;
	rec.audioCodecCtx->sample_rate = 48000;
	rec.audioCodecCtx->sample_fmt  = AV_SAMPLE_FMT_FLTP;
	rec.audioCodecCtx->ch_layout =
		(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	rec.audioCodecCtx->bit_rate = 128000;
	/* Some formats want stream headers to be separate. */
	if (rec.outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		rec.audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	avcodec_open2(rec.audioCodecCtx, audioCodec, NULL);
	if (ret < 0) {
		warning("avcodec_open2 audio error\n");
		return EINVAL;
	}
	avcodec_parameters_from_context(rec.audioStream->codecpar,
					rec.audioCodecCtx);

	ret = avio_open(&rec.outputFormatContext->pb, rec.filename,
			AVIO_FLAG_WRITE);
	if (ret < 0) {
		warning("avio_open error\n");
		return EINVAL;
	}

	ret = avformat_write_header(rec.outputFormatContext, NULL);
	if (ret < 0) {
		warning("avformat_write_header error\n");
		return EINVAL;
	}

	rec.videoFrame	       = av_frame_alloc();
	rec.videoFrame->width  = conf->video.width;
	rec.videoFrame->height = conf->video.height;
	rec.videoFrame->format = AV_PIX_FMT_YUV420P;

	init_resampler();

#if STREAM
	init_stream();
#endif

	re_atomic_rlx_set(&rec.run, true);
	info("vidmix: record started\n");

	thread_create_name(&rec.thread, "vidrec", record_thread, NULL);

	return 0;
}


void vmix_audio_record(struct auframe *af);
void vmix_audio_record(struct auframe *af)
{
	if (!re_atomic_rlx(&rec.run) || !re_atomic_rlx(&rec.video_start_time))
		return;

	if (!re_atomic_rlx(&rec.audio_start_time))
		re_atomic_rlx_set(&rec.audio_start_time, af->timestamp);

	aubuf_write_auframe(rec.ab, af);
}


static void record_destroy(void *arg)
{
	struct record_entry *e = arg;

	mem_deref(e->frame);

	mtx_lock(rec.lock);
	list_unlink(&e->le);
	mtx_unlock(rec.lock);
}


int vmix_record(const struct vidframe *frame, uint64_t ts)
{
	if (!re_atomic_rlx(&rec.run))
		return 0;

	if (!re_atomic_rlx(&rec.video_start_time))
		re_atomic_rlx_set(&rec.video_start_time, ts);

	struct record_entry *e =
		mem_zalloc(sizeof(struct record_entry), record_destroy);
	if (!e)
		return ENOMEM;

	struct vidsz vsz = {.w = 1920, .h = 1080};

	int err = vidframe_alloc(&e->frame, VID_FMT_YUV420P, &vsz);
	if (unlikely(err)) {
		mem_deref(e);
		return ENOMEM;
	}

	vidframe_copy(e->frame, frame);
	e->ts = ts;

	mtx_lock(rec.lock);
	list_append(&rec.vframes, &e->le, e);
	mtx_unlock(rec.lock);

	return 0;
}


int vmix_record_close(void)
{
	if (!re_atomic_rlx(&rec.run))
		return EINVAL;

	re_atomic_rlx_set(&rec.run, false);
	thrd_join(rec.thread, NULL);

	re_atomic_rlx_set(&rec.audio_start_time, 0);
	re_atomic_rlx_set(&rec.video_start_time, 0);

#if STREAM
	close_stream();
#endif

	/* Write the trailer and close the output file */
	av_write_trailer(rec.outputFormatContext);
	avio_close(rec.outputFormatContext->pb);

	/* Clean up */
	avcodec_free_context(&rec.videoCodecCtx);
	avcodec_free_context(&rec.audioCodecCtx);
	avformat_free_context(rec.outputFormatContext);
	swr_free(&rec.resample_context);

	uint32_t count = list_count(&rec.vframes);
	if (count)
		warning("rec/close: drop %u frames\n", count);

	list_flush(&rec.vframes);

	rec.ab = mem_deref(rec.ab);

	mem_deref(rec.lock);
	av_frame_free(&rec.videoFrame);

	chmod(rec.filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	return 0;
}
