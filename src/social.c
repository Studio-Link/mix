#include <mix.h>

#define ACTIVITY_JSON "Accept: application/activity+json"

enum {
	HTTP_SCHEME_SZ	 = sizeof("http://") - 1,
	HTTPS_SCHEME_SZ	 = sizeof("https://") - 1,
	ACTIVITY_JSON_SZ = sizeof(ACTIVITY_JSON) - 1,
};

struct social {
	struct session *sess;
	struct http_conn *oreq;
	struct mbuf *mb;
	struct sl_httpconn *slconn;
	char *name;
	char *summary;
};

static void avatarsavedh(int err, void *arg)
{
	struct social *social = arg;

	if (err)
		goto out;

	char *status = NULL;
	re_sdprintf(&status,
		    "{\"status\": 200, \"id\": \"%s\", \"name\": "
		    "\"%s\", \"summary\": \"%H\" }",
		    social->sess->user->id, social->name, utf8_encode,
		    social->summary);
	http_sreply(social->oreq, 200, "OK", "application/json", status,
		    str_len(status), social->sess);
	mem_deref(status);
	social->oreq = mem_deref(social->oreq);

out:
	mem_deref(social);
}


static void avatarh(int err, const struct http_msg *msg, void *arg)
{
	struct social *social = arg;

	if (err)
		goto out;

	err = avatar_save(social->sess, social->oreq, msg, avatarsavedh,
			  social);

out:
	if (err)
		mem_deref(social);
}


static void userh(int err, const struct http_msg *msg, void *arg)
{
	struct social *social = arg;
	struct odict *o	      = NULL;
	char *json	      = NULL;

	if (err) {
		warning("userh: http error %m\n", err);
		goto out;
	}

	re_sdprintf(&json, "%b", mbuf_buf(msg->mb), mbuf_get_left(msg->mb));
	err = json_decode_odict(&o, 32, json, str_len(json), 8);
	if (err) {
		warning("userh: json error %m\n", err);
		goto out;
	}

	str_dup(&social->name, odict_string(o, "name"));
	str_dup(&social->summary, odict_string(o, "summary"));

	struct odict *icon = odict_get_object(o, "icon");
	if (!icon) {
		warning("userh: icon not found\n");
		err = ENODATA;
		goto out;
	}

	const char *url = odict_string(icon, "url");
	if (!url) {
		warning("userh: url not found\n");
		err = ENODATA;
		goto out;
	}

	social->slconn = mem_deref(social->slconn);

	err = sl_httpc_alloc(&social->slconn, avatarh, NULL, social);
	if (err)
		goto out;

	err = sl_httpc_req(social->slconn, SL_HTTP_GET, url, NULL);

out:
	mem_deref(o);
	mem_deref(json);

	if (err)
		mem_deref(social);
}


static void webfingerh(int err, const struct http_msg *msg, void *arg)
{
	struct social *social = arg;
	struct odict *o	      = NULL;
	char *json	      = NULL;
	const char *href      = NULL;

	if (err) {
		warning("webfingerh: http error %m\n", err);
		goto out;
	}

	re_sdprintf(&json, "%b", mbuf_buf(msg->mb), mbuf_get_left(msg->mb));
	err = json_decode_odict(&o, 32, json, str_len(json), 8);
	if (err) {
		warning("webfingerh: json error %m\n", err);
		goto out;
	}

	err = ENODATA;

	struct odict *links = odict_get_array(o, "links");
	if (!links)
		goto out;

	for (struct le *le = links->lst.head; le; le = le->next) {
		const struct odict_entry *e = le->data;

		struct odict *oo = odict_entry_object(e);
		const char *rel	 = odict_string(oo, "rel");
		if (str_casecmp(rel, "self") == 0) {
			href = odict_string(oo, "href");
			err  = 0;
			break;
		}
	}

	if (err)
		goto out;

	social->slconn = mem_deref(social->slconn);

	err = sl_httpc_alloc(&social->slconn, userh, NULL, social);
	if (err)
		goto out;

	struct pl header = {.p = ACTIVITY_JSON, .l = ACTIVITY_JSON_SZ};

	err = http_reqconn_add_header(social->slconn->conn, &header);
	if (err)
		goto out;

	err = sl_httpc_req(social->slconn, SL_HTTP_GET, href, NULL);

out:
	mem_deref(o);
	mem_deref(json);

	if (err)
		mem_deref(social);
}


static void social_destruct(void *arg)
{
	struct social *social = arg;

	if (social->oreq) {
		char status[] = "{\"status\": 404 }";
		http_sreply(social->oreq, 200, "OK", "application/json",
			    status, sizeof(status) - 1, social->sess);
		mem_deref(social->oreq);
	}
	mem_deref(social->slconn);
	mem_deref(social->sess);
	mem_deref(social->name);
	mem_deref(social->summary);
}


int social_request(struct http_conn *conn, const struct http_msg *msg,
		   struct session *sess)
{
	struct social *social;
	struct uri uri;
	struct pl req_uri = PL_INIT;
	char buf[256]	  = {0};
	char url[256]	  = {0};
	int err;

	social = mem_zalloc(sizeof(struct social), social_destruct);
	if (!social)
		return ENOMEM;

	social->oreq = mem_ref(conn);
	social->sess = mem_ref(sess);

	pl_set_mbuf(&req_uri, msg->mb);

	if (pl_strncasecmp(&req_uri, "http://", HTTP_SCHEME_SZ) == 0) {
		req_uri.p += HTTP_SCHEME_SZ;
		req_uri.l -= HTTP_SCHEME_SZ;
	}

	if (pl_strncasecmp(&req_uri, "https://", HTTPS_SCHEME_SZ) == 0) {
		req_uri.p += HTTPS_SCHEME_SZ;
		req_uri.l -= HTTPS_SCHEME_SZ;
	}

	if (pl_strncmp(&req_uri, "@", 1) == 0) {
		req_uri.p += 1;
		req_uri.l -= 1;
	}

	re_snprintf(buf, sizeof(buf), "http:%r", &req_uri);

	pl_set_str(&req_uri, buf);

	err = uri_decode(&uri, &req_uri);
	if (err)
		goto out;

	re_snprintf(url, sizeof(url),
		    "https://%r/.well-known/webfinger?resource=acct:%r@%r",
		    &uri.host, &uri.user, &uri.host);

	warning("webfinger -> '%s' %m\n", url, err);

	err = sl_httpc_alloc(&social->slconn, webfingerh, NULL, social);
	if (err)
		goto out;

	err = sl_httpc_req(social->slconn, SL_HTTP_GET, url, NULL);

out:
	if (err)
		mem_deref(social);

	return err;
}
