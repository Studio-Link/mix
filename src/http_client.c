#include <mix.h>

static struct http_cli *client = NULL;


static void destroy(void *arg)
{
	struct sl_httpconn *p = arg;
	mem_deref(p->conn);
}


int sl_httpc_init(void)
{
	int err;

	err = http_client_alloc(&client, net_dnsc(baresip_network()));
	if (err)
		return err;

	err = http_client_add_ca(client, "/etc/ssl/certs/ca-certificates.crt");

	return err;
}


int sl_httpc_alloc(struct sl_httpconn **slconn, http_resp_h *resph,
		   http_data_h *datah, void *arg)
{
	int err;
	struct sl_httpconn *p;

	if (!slconn || !client)
		return EINVAL;

	p = mem_zalloc(sizeof(struct sl_httpconn), destroy);
	if (!p)
		return ENOMEM;

	p->arg = arg;

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

	pl_set_str(&uri, url);
	return http_reqconn_send(slconn->conn, &uri);
}


void sl_httpc_close(void)
{
	client = mem_deref(client);
}
