#include <re.h>
#include <baresip.h>
#include "mix.h"

enum { MAX_LISTENERS = 20 };


int users_json(char **json, struct mix *mix)
{
	struct le *le;
	struct odict *o;
	struct odict *o_user;
	int err;

	if (!json || !mix)
		return EINVAL;

	err = odict_alloc(&o, 32);
	if (err)
		return err;

	//@FIXME: add sorting
	int listeners = 0;
	LIST_FOREACH(&mix->sessl, le)
	{
		struct session *sess = le->data;

		if (listeners++ > MAX_LISTENERS)
			continue;

		err = odict_alloc(&o_user, 8);
		if (err)
			goto out;

		odict_entry_add(o_user, "id", ODICT_STRING, sess->user_id);
		odict_entry_add(o_user, "name", ODICT_STRING, sess->name);
		odict_entry_add(o_user, "speaker", ODICT_BOOL, false);
		odict_entry_add(o_user, "admin", ODICT_BOOL, true);

		odict_entry_add(o, sess->user_id, ODICT_OBJECT, o_user);
		o_user = mem_deref(o_user);
	}

	/* odict_entry_add(o, "extra_listeners", ODICT_INT, listeners); */

	err = re_sdprintf(json, "%H", json_encode_odict, o);

out:
	mem_deref(o);
	return err;
}
