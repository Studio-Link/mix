#include "mix.h"


static void destructor(void *data)
{
	struct chat *chat = data;

	list_unlink(&chat->le);
	mem_deref(chat->user);
}


int chat_save(struct user *user, struct mix *mix, const struct http_msg *msg)
{
	struct chat *chat;
	char *json;
	int err;

	if (!user || !mix || !msg || !str_isset(user->id))
		return EINVAL;

	if (mbuf_get_left(msg->mb) < 2) {
		return ENODATA;
	}

	if (mbuf_get_left(msg->mb) >= sizeof(chat->message)) {
		warning("chat_save: message to big\n");
		return EOVERFLOW;
	}

	chat = mem_zalloc(sizeof(struct chat), destructor);
	if (!chat)
		return ENOMEM;

	chat->user = mem_ref(user);
	chat->time = tmr_jiffies_rt_usec() / 1000 / 1000;

	msg->mb->pos += 1;
	msg->mb->end -= 1;

	mbuf_read_mem(msg->mb, (uint8_t *)chat->message,
		      mbuf_get_left(msg->mb));

	list_append(&mix->chatl, &chat->le, chat);

	err = chat_event_json(&json, CHAT_ADDED, chat);
	if (err)
		return err;

	sl_ws_send_event_all(json);

	mem_deref(json);

	return 0;
}


int chat_json(char **json, struct mix *mix)
{
	struct le *le;
	struct odict *o;
	struct odict *o_chat;
	struct odict *o_chats;
	char time_str[ITOA_BUFSZ];
	int err;

	if (!json || !mix)
		return EINVAL;

	err = odict_alloc(&o, 32);
	if (err)
		return err;

	err = odict_alloc(&o_chats, 32);
	if (err)
		return err;

	LIST_FOREACH(&mix->chatl, le)
	{
		struct chat *chat = le->data;

		err = odict_alloc(&o_chat, 8);
		if (err)
			goto out;

		odict_entry_add(o_chat, "user_id", ODICT_STRING,
				chat->user->id);
		odict_entry_add(o_chat, "user_name", ODICT_STRING,
				chat->user->name);
		odict_entry_add(
			o_chat, "time", ODICT_STRING,
			str_itoa((uint32_t)(chat->time), time_str, 10));
		odict_entry_add(o_chat, "msg", ODICT_STRING, chat->message);

		odict_entry_add(o_chats, "", ODICT_OBJECT, o_chat);
		o_chat = mem_deref(o_chat);
	}

	odict_entry_add(o, "chats", ODICT_ARRAY, o_chats);

	err = re_sdprintf(json, "%H", json_encode_odict, o);

out:
	mem_deref(o);
	mem_deref(o_chats);
	return err;
}


int chat_event_json(char **json, enum user_event event, struct chat *chat)
{

	struct odict *o;
	int err;
	char time_str[ITOA_BUFSZ];

	if (!json || !chat)
		return EINVAL;

	err = odict_alloc(&o, 8);
	if (err)
		return err;

	odict_entry_add(o, "type", ODICT_STRING, "chat");

	if (event == CHAT_ADDED)
		odict_entry_add(o, "event", ODICT_STRING, "chat_added");
	else {
		err = EINVAL;
		goto out;
	}

	odict_entry_add(o, "user_id", ODICT_STRING, chat->user->id);
	odict_entry_add(o, "user_name", ODICT_STRING, chat->user->name);
	odict_entry_add(o, "time", ODICT_STRING,
			str_itoa((uint32_t)(chat->time), time_str, 10));
	odict_entry_add(o, "msg", ODICT_STRING, chat->message);

	err = re_sdprintf(json, "%H", json_encode_odict, o);

out:
	mem_deref(o);

	return err;
}
