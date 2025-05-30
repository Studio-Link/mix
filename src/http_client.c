#include <mix.h>

static struct http_cli *client = NULL;

struct http_conf sl_http_conf = {.conn_timeout = 10 * 1000,
				 .recv_timeout = 60 * 1000,
				 .idle_timeout = 900 * 1000};

enum { HTTP_MAX_REDIRECTS = 10 };

static void destroy(void *arg)
{
	struct sl_httpconn *p = arg;
	mem_deref(p->conn);
}


static void resph(int err, const struct http_msg *msg, void *arg)
{
	struct sl_httpconn *p = arg;
	char *url	      = NULL;

	if (!err && msg && msg->scode >= 301 && msg->scode <= 308) {
		const struct http_hdr *location =
			http_msg_hdr(msg, HTTP_HDR_LOCATION);

		if (++p->redirects > HTTP_MAX_REDIRECTS) {
			err = E2BIG;
			goto err;
		}

		err = pl_strdup(&url, &location->val);
		if (err)
			goto err;

		sl_httpc_req(p, SL_HTTP_GET, url, NULL);
		if (err)
			goto err;

		goto out;
	}

err:
	if (p->slresph)
		p->slresph(err, msg, p->arg);

out:
	mem_deref(p);
	mem_deref(url);
}


int sl_httpc_alloc(struct sl_httpconn **slconn, http_resp_h *slresph,
		   http_data_h *datah, void *arg)
{
	int err;
	struct sl_httpconn *p;

	if (!slconn || !client)
		return EINVAL;

	p = mem_zalloc(sizeof(struct sl_httpconn), destroy);
	if (!p)
		return ENOMEM;

	p->arg	   = arg;
	p->slresph = slresph;

	err = http_reqconn_alloc(&p->conn, client, resph, datah, p);
	if (err)
		mem_deref(p);
	else
		*slconn = p;

	return err;
}


int sl_httpc_req(struct sl_httpconn *slconn, enum sl_httpc_met sl_met,
		 const char *url, struct mbuf *body)
{
	struct pl uri, met;
	int err;

	if (!slconn || !url || !client)
		return EINVAL;

	switch (sl_met) {
	case SL_HTTP_GET:
		pl_set_str(&met, "GET");
		break;
	case SL_HTTP_POST:
		pl_set_str(&met, "POST");
		break;
	case SL_HTTP_PUT:
		pl_set_str(&met, "PUT");
		break;
	case SL_HTTP_PATCH:
		pl_set_str(&met, "PATCH");
		break;
	case SL_HTTP_DELETE:
		pl_set_str(&met, "DELETE");
		break;
	default:
		return ENOTSUP;
	}

	err = http_reqconn_set_method(slconn->conn, &met);
	if (err)
		return err;

	if (body) {
		err = http_reqconn_set_body(slconn->conn, body);
		if (err)
			return err;
	}

	mem_ref(slconn);
	pl_set_str(&uri, url);
	return http_reqconn_send(slconn->conn, &uri);
}


int sl_httpc_init(void)
{
	int err;

	err = http_client_alloc(&client, net_dnsc(baresip_network()));
	if (err)
		return err;

	err = http_client_add_ca(client, "/etc/ssl/certs/ca-certificates.crt");
	if (err)
		return err;

	err = http_client_set_config(client, &sl_http_conf);

	return err;
}


void sl_httpc_close(void)
{
	client = mem_deref(client);
}
