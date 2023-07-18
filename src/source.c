#include <mix.h>

static int ws_json(struct session *sess, const struct odict *od)
{
	char *buf = NULL;
	int err;

	if (!sess)
		return EINVAL;

	err = re_sdprintf(&buf, "%H", json_encode_odict, od);
	if (err)
		goto out;

	sl_ws_send_event_self(sess, buf);

out:
	mem_deref(buf);

	return err;
}


static void gather_handler(void *arg)
{
	struct source_pc *src = arg;
	struct mbuf *mb_sdp   = NULL;
	struct odict *od      = NULL;
	enum sdp_type type    = SDP_NONE;
	int err;

	switch (peerconnection_signaling(src->pc)) {

	case SS_STABLE:
		type = SDP_OFFER;
		break;

	case SS_HAVE_LOCAL_OFFER:
		warning("sip gather illegal state\n");
		type = SDP_OFFER;
		break;

	case SS_HAVE_REMOTE_OFFER:
		type = SDP_ANSWER;
		break;
	}

	info("source: session gathered -- send sdp '%s'\n",
	     sdptype_name(type));

	if (type == SDP_OFFER)
		err = peerconnection_create_offer(src->pc, &mb_sdp);
	else
		err = peerconnection_create_answer(src->pc, &mb_sdp);
	if (err)
		goto out;

	err = session_description_encode(&od, type, mb_sdp);
	if (err) {
		warning("source: sdp encode error: %m\n", err);
		goto out;
	}

	err = odict_entry_add(od, "id", ODICT_INT, src->id);
	err = odict_entry_add(od, "dev", ODICT_STRING, src->dev);
	if (err) {
		warning("source: odict id error: %m\n", err);
		goto out;
	}

	err = ws_json(src->sess, od);
	if (err) {
		warning("source: reply ws error: %m\n", err);
		goto out;
	}

	if (type == SDP_ANSWER) {
		err = peerconnection_start_ice(src->pc);
		if (err) {
			warning("source: failed to start ice (%m)\n", err);
			goto out;
		}
	}

out:
	mem_deref(mb_sdp);
	mem_deref(od);
}


static void estab_handler(struct media_track *media, void *arg)
{
	int err		      = 0;
	struct source_pc *src = arg;

	info("source: webrtc stream established: '%s'\n",
	     media_kind_name(mediatrack_kind(media)));

	switch (mediatrack_kind(media)) {

	case MEDIA_KIND_AUDIO:
		err = mediatrack_start_audio(media, baresip_ausrcl(),
					     baresip_aufiltl());
		if (err) {
			warning("source: could not start audio (%m)\n", err);
		}
		break;

	case MEDIA_KIND_VIDEO:
		video_set_devicename(media_get_video(media), src->source_dev,
				     "dummy");
		err = mediatrack_start_video(media);
		if (err) {
			warning("source: could not start video (%m)\n", err);
		}
		break;

	default:
		break;
	}

	if (err)
		return;

	stream_enable(media_get_stream(media), true);
}


static void close_handler(int err, void *arg)
{

	struct source_pc *src = arg;
	(void)err;

	mem_deref(src);
}


static void source_dealloc(void *arg)
{
	struct source_pc *src = arg;
	struct odict *od;

	list_unlink(&src->le);

	int err = odict_alloc(&od, 1);
	if (!err) {
		odict_entry_add(od, "type", ODICT_STRING, "source_close");
		odict_entry_add(od, "id", ODICT_INT, src->id);
		ws_json(src->sess, od);
		mem_deref(od);
	}

	src->pc = mem_deref(src->pc);
}


static int32_t source_id_next(struct source_pc *src)
{
	if (!src || !src->sess)
		return -1;

	if (!src->sess->source_pcl.tail)
		return 0;

	struct source_pc *last = src->sess->source_pcl.tail->data;

	if (!last)
		return -1;

	return last->id + 1;
}


int slmix_source_alloc(struct source_pc **srcp, struct session *sess,
		       const char *dev)
{
	int err = 0;

	struct source_pc *src =
		mem_zalloc(sizeof(struct source_pc), source_dealloc);
	if (!src)
		return ENOMEM;

	src->sess = sess;
	re_snprintf(src->source_dev, sizeof(src->source_dev), "pktsrc%s", dev);
	re_snprintf(src->dev, sizeof(src->dev), "%s", dev);

	src->id = source_id_next(src);
	if (src->id == -1) {
		warning("source: set source id failed!\n");
		err = EINVAL;
		goto out;
	}

out:
	if (err)
		mem_deref(src);
	else
		*srcp = src;

	return err;
}


int slmix_source_start(struct source_pc *src, struct mix *mix)
{
	struct config config		   = *conf_config();
	struct rtc_configuration pc_config = {.offerer = true};
	int err;

	re_snprintf(config.video.src_mod, sizeof(config.video.src_mod),
		    "vmix_pktsrc");

	src->pc = mem_deref(src->pc); /* clear old connection */

	err = peerconnection_new(&src->pc, &pc_config, mix->mnat, mix->menc,
				 gather_handler, estab_handler, close_handler,
				 src);
	if (err) {
		warning("source: peerconnection failed (%m)\n", err);
		return err;
	}

	err = peerconnection_add_audio_track(src->pc, &config,
					     baresip_aucodecl(), SDP_SENDONLY);
	if (err) {
		warning("source: add_audio failed (%m)\n", err);
		return err;
	}

	err = peerconnection_add_video_track(
		src->pc, &config, baresip_vidcodecl(), SDP_SENDONLY);
	if (err) {
		warning("source: add_video failed (%m)\n", err);
		return err;
	}

	return 0;
}


int slmix_handle_ice_candidate(struct peer_connection *pc,
			       const struct odict *od)
{
	const char *cand, *mid;
	struct pl pl_cand;
	char *cand2 = NULL;
	int err;

	cand = odict_string(od, "candidate");
	mid  = odict_string(od, "sdpMid");
	if (!cand || !mid) {
		warning("mix: candidate: missing 'candidate' or "
			"'mid'\n");
		return EPROTO;
	}

	err = re_regex(cand, str_len(cand), "candidate:[^]+", &pl_cand);
	if (err)
		return err;

	pl_strdup(&cand2, &pl_cand);

	peerconnection_add_ice_candidate(pc, cand2, mid);

	mem_deref(cand2);

	return 0;
}
