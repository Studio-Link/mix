#include <re.h>
#include <baresip.h>
#include <mix.h>

#define SL_MAX_TRACKS 16

static struct list tracks = LIST_INIT;

/* TODO: refactor allow multiple local tracks */
static struct sl_track *local_track = NULL;


const struct list *sl_tracks(void)
{
	return &tracks;
}


int sl_tracks_json(struct re_printf *pf, void *arg)
{
	struct le *le;
	struct odict *o_tracks;
	struct odict *o_track;
	char id[ITOA_BUFSZ];
	int err;
	(void)arg;

	if (!pf)
		return EINVAL;

	err = odict_alloc(&o_tracks, 32);
	if (err)
		return ENOMEM;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (!track)
			continue;

		err = odict_alloc(&o_track, 32);
		if (err)
			return ENOMEM;

		if (track->type == SL_TRACK_REMOTE) {
			odict_entry_add(o_track, "type", ODICT_STRING,
					"remote");
		}

		odict_entry_add(o_track, "name", ODICT_STRING, track->name);
		odict_entry_add(o_track, "status", ODICT_INT, track->status);
		odict_entry_add(o_track, "error", ODICT_STRING, track->error);
		odict_entry_add(o_track, "muted", ODICT_BOOL, track->muted);
		odict_entry_add(o_tracks, str_itoa(track->id, id, 10),
				ODICT_OBJECT, o_track);
		o_track = mem_deref(o_track);
	}

	err = json_encode_odict(pf, o_tracks);
	mem_deref(o_tracks);

	return err;
}


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct sl_track *track1 = le1->data;
	struct sl_track *track2 = le2->data;
	(void)arg;

	/* NOTE: important to use less than OR equal to, otherwise
	   the list_sort function may be stuck in a loop */
	return track1->id <= track2->id;
}


int sl_track_next_id(void)
{
	int id = 1;
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			++id;
			continue;
		}
		break;
	}

	return id;
}


static void track_destructor(void *data)
{
	struct sl_track *track = data;

	list_unlink(&track->le);

	if (track->type == SL_TRACK_LOCAL)
		mem_deref(track->u.local.slaudio);

	if (track->type == SL_TRACK_REMOTE) {
		if (track->u.remote.call)
			ua_hangup(slmix_sip_ua(), track->u.remote.call, 0,
				  NULL);
	}
}


int sl_track_add(struct sl_track **trackp, enum sl_track_type type)
{
	struct sl_track *track;

	if (!trackp)
		return EINVAL;

	if (list_count(&tracks) >= SL_MAX_TRACKS) {
		warning("sl_track_add: max. %d tracks reached\n",
			SL_MAX_TRACKS);
		return E2BIG;
	}

	track = mem_zalloc(sizeof(struct sl_track), track_destructor);
	if (!track)
		return ENOMEM;

	track->id     = sl_track_next_id();
	track->type   = type;
	track->status = SL_TRACK_IDLE;
	track->muted  = false;

	list_append(&tracks, &track->le, track);
	list_sort(&tracks, sort_handler, NULL);

	*trackp = track;

	return 0;
}


int sl_track_del(int id)
{
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			mem_deref(track);
			return 0;
		}
	}
	return ENOENT;
}


struct sl_track *sl_track_by_id(int id)
{
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			return track;
		}
	}
	return NULL;
}


enum sl_track_status sl_track_status(int id)
{
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id)
			return track->status;
	}

	return SL_TRACK_INVALID;
}


struct slaudio *sl_track_audio(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_LOCAL)
		return NULL;

	return track->u.local.slaudio;
}


int sl_track_dial(struct sl_track *track, struct pl *peer)
{
	int err;
	char *peerc = NULL;

	if (!track || track->type != SL_TRACK_REMOTE)
		return EINVAL;

	track->error[0] = '\0';

	const struct contacts *cs = baresip_contacts();

	for (struct le *le = list_head(contact_list(cs)); le; le = le->next) {
		struct contact *c  = le->data;
		struct sip_addr *a = contact_addr(c);
		if (!a)
			continue;

		if (pl_casecmp(peer, &a->dname) == 0) {
			str_dup(&peerc, contact_uri(c));
			break;
		}
	}

	if (!peerc) {
		err = account_uri_complete_strdup(ua_account(slmix_sip_ua()),
						  &peerc, peer);
		if (err)
			goto out;
	}

	err = ua_connect(slmix_sip_ua(), &track->u.remote.call, NULL, peerc,
			 VIDMODE_OFF);
	if (err)
		goto out;

	track->status = SL_TRACK_REMOTE_CALLING;
	pl_strcpy(peer, track->name, sizeof(track->name));

out:
	if (err)
		re_snprintf(track->error, sizeof(track->error), "%m", err);

	if (err == EINVAL)
		str_ncpy(track->error, "Invalid ID", sizeof(track->error));


	sl_track_ws_send();

	mem_deref(peerc);

	return err;
}


void sl_track_accept(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_REMOTE)
		return;

	ua_answer(call_get_ua(track->u.remote.call), track->u.remote.call,
		  VIDMODE_OFF);
}


void sl_track_hangup(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_REMOTE)
		return;

	ua_hangup(call_get_ua(track->u.remote.call), track->u.remote.call, 0,
		  "");

	track->name[0]	     = '\0';
	track->u.remote.call = NULL;
}


void sl_track_toggle_mute(struct sl_track *track)
{
	/* only allowed for local track currently */
	if (!track || track->id != 1)
		return;

	local_track->muted = !local_track->muted;
	sl_track_ws_send();
}


void sl_track_ws_send(void)
{
	char *json_str;
	re_sdprintf(&json_str, "%H", sl_tracks_json, NULL);
	sl_ws_send_event_host(WS_TRACKS, json_str);
	mem_deref(json_str);
}


static void call_incoming(struct call *call)
{
	struct le *le;
	struct sl_track *track;

	if (!call)
		return;

	LIST_FOREACH(&tracks, le)
	{
		track = le->data;

		if (track->type != SL_TRACK_REMOTE)
			continue;

		if (track->u.remote.call)
			continue;

		goto out;
	}

	/* Add new track if no empty remote track is found */
	sl_track_add(&track, SL_TRACK_REMOTE);

out:
	track->u.remote.call = call;
	track->status	     = SL_TRACK_REMOTE_INCOMING;
	str_ncpy(track->name, call_peeruri(call), sizeof(track->name));
	char buf[ITOA_BUFSZ];
	char *id = str_itoa(track->id, buf, 10);
	audio_set_devicename(call_audio(call), id, id);
}


static void eventh(enum bevent_ev ev, struct bevent *event, void *arg)
{
	struct le *le;
	bool changed		  = false;
	struct call *call	  = bevent_get_call(event);
	const char *prm		  = bevent_get_text(event);
	const struct sip_msg *msg = bevent_get_msg(event);
	(void)arg;

	if (ev == BEVENT_SIPSESS_CONN) {
		struct ua *ua = uag_find_msg(msg);
		ua_accept(ua, msg);
		bevent_stop(event);
		return;
	}

	if (ev == BEVENT_CALL_INCOMING) {
		call_incoming(call);
		sl_track_ws_send();
		return;
	}

	if (ev == BEVENT_REGISTERING) {
		if (local_track) {
			str_ncpy(local_track->name,
				 account_aor(ua_account(slmix_sip_ua())),
				 sizeof(local_track->name));
			local_track->status = SL_TRACK_LOCAL_REGISTERING;
			sl_track_ws_send();
		}
		return;
	}

	if (ev == BEVENT_REGISTER_OK) {
		if (local_track) {
			local_track->status = SL_TRACK_LOCAL_REGISTER_OK;
			sl_track_ws_send();
		}
		return;
	}

	if (ev == BEVENT_REGISTER_FAIL) {
		if (local_track) {
			local_track->status = SL_TRACK_LOCAL_REGISTER_FAIL;
			sl_track_ws_send();
		}
		return;
	}

	if (ev == BEVENT_SHUTDOWN) {
		list_flush(&tracks);
		return;
	}

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;

		if (track->type != SL_TRACK_REMOTE)
			continue;

		if (track->u.remote.call != call)
			continue;


		if (ev == BEVENT_CALL_RINGING) {
			track->status = SL_TRACK_REMOTE_CALLING;
			changed	      = true;
		}

		if (ev == BEVENT_CALL_ESTABLISHED) {
			track->status = SL_TRACK_REMOTE_CONNECTED;
			changed	      = true;
		}

		if (ev == BEVENT_CALL_CLOSED) {
			track->status	     = SL_TRACK_IDLE;
			track->u.remote.call = NULL;
			track->name[0]	     = '\0';
			if (call_scode(call) != 200)
				str_ncpy(track->error, prm,
					 sizeof(track->error));
			changed = true;
		}
	}

	if (!changed)
		return;

	sl_track_ws_send();
}


int sl_tracks_init(void)
{
	bevent_register(eventh, NULL);

	return 0;
}


int sl_tracks_close(void)
{
	bevent_unregister(eventh);
	list_flush(&tracks);

	local_track = NULL;

	return 0;
}
