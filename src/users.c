#include <re.h>
#include <baresip.h>
#include "mix.h"

enum { MAX_LISTENERS = 30 };


int users_json(char **json, struct mix *mix)
{
	struct le *le;
	struct odict *o;
	struct odict *o_user;
	struct odict *o_users;
	int err;

	if (!json || !mix)
		return EINVAL;

	err = odict_alloc(&o, 8);
	if (err)
		return err;

	err = odict_alloc(&o_users, 32);
	if (err)
		return err;

	int listeners = 0;
	LIST_FOREACH(&mix->sessl, le)
	{
		struct session *sess = le->data;

		if (!sess->connected)
			continue;

		if (!sess->user->speaker)
			++listeners;

		if (listeners > MAX_LISTENERS)
			continue;

		err = odict_alloc(&o_user, 8);
		if (err)
			goto out;

		odict_entry_add(o_user, "id", ODICT_STRING, sess->user->id);
		odict_entry_add(o_user, "pidx", ODICT_INT, sess->user->pidx);
		odict_entry_add(o_user, "name", ODICT_STRING,
				sess->user->name);
		odict_entry_add(o_user, "speaker", ODICT_BOOL,
				sess->user->speaker);
		odict_entry_add(o_user, "host", ODICT_BOOL, sess->user->host);
		odict_entry_add(o_user, "video", ODICT_BOOL,
				sess->user->video);
		odict_entry_add(o_user, "audio", ODICT_BOOL,
				sess->user->audio);
		odict_entry_add(o_user, "hand", ODICT_BOOL, sess->user->hand);
		odict_entry_add(o_user, "webrtc", ODICT_BOOL,
				sess->pc ? true : false);

		odict_entry_add(o_users, sess->user->id, ODICT_OBJECT, o_user);
		o_user = mem_deref(o_user);
	}

	odict_entry_add(o, "type", ODICT_STRING, "users");
	odict_entry_add(o, "listeners", ODICT_INT, listeners);
	odict_entry_add(o, "users", ODICT_ARRAY, o_users);

	err = re_sdprintf(json, "%H", json_encode_odict, o);

out:
	mem_deref(o);
	mem_deref(o_users);
	return err;
}


int user_event_json(char **json, enum user_event event, struct session *sess)
{
	struct odict *o;
	int err;

	if (!json || !sess)
		return EINVAL;

	err = odict_alloc(&o, 8);
	if (err)
		return err;

	odict_entry_add(o, "type", ODICT_STRING, "user");

	if (event == USER_ADDED)
		odict_entry_add(o, "event", ODICT_STRING, "added");
	else if (event == USER_UPDATED)
		odict_entry_add(o, "event", ODICT_STRING, "updated");
	else if (event == USER_DELETED)
		odict_entry_add(o, "event", ODICT_STRING, "deleted");
	else {
		err = EINVAL;
		goto out;
	}

	odict_entry_add(o, "id", ODICT_STRING, sess->user->id);
	odict_entry_add(o, "pidx", ODICT_INT, sess->user->pidx);
	odict_entry_add(o, "name", ODICT_STRING, sess->user->name);
	odict_entry_add(o, "speaker", ODICT_BOOL, sess->user->speaker);
	odict_entry_add(o, "host", ODICT_BOOL, sess->user->host);
	odict_entry_add(o, "video", ODICT_BOOL, sess->user->video);
	odict_entry_add(o, "audio", ODICT_BOOL, sess->user->audio);
	odict_entry_add(o, "hand", ODICT_BOOL, sess->user->hand);
	odict_entry_add(o, "webrtc", ODICT_BOOL, sess->pc ? true : false);

	err = re_sdprintf(json, "%H", json_encode_odict, o);

out:
	mem_deref(o);

	return err;
}
