#include <re.h>
#include <baresip.h>
#include <gd.h>
#include <stdint.h>
#include <stdio.h>

#include "mix.h"

struct avatar {
	struct http_conn *conn;
	const struct http_msg *msg;
};


static int work(void *arg)
{
	struct avatar *avatar = arg;
	struct mbuf *mb	      = avatar->msg->mb;
	FILE *jpg;
	int w = 128;
	int h = 128;
	gdImagePtr in, out;
	struct pl pl = PL_INIT;

	pl.l = mbuf_get_left(mb) * 2;
	pl.p = mem_zalloc(pl.l, NULL);

	FILE *tmp_png = tmpfile();

	mbuf_advance(mb, 0x17);

	base64_decode((char *)mbuf_buf(mb), mbuf_get_left(mb) - 1,
		      (uint8_t *)pl.p, &pl.l);

	fwrite(pl.p, pl.l, 1, tmp_png);

	mem_deref((void *)pl.p);
	rewind(tmp_png);

	in = gdImageCreateFromPng(tmp_png);
	fclose(tmp_png);
	if (!in) {
		warning("avatar: create image failed\n");
		return EIO;
	}

	gdImageSetInterpolationMethod(in, GD_BILINEAR_FIXED);
	out = gdImageScale(in, w, h);
	if (!out) {
		gdImageDestroy(in);
		warning("avatar: image scale failed\n");
		return EIO;
	}

	fs_fopen(&jpg, "/tmp/test.jpg", "w+");
	gdImageJpeg(out, jpg, 90);
	fclose(jpg);

	gdImageDestroy(in);
	gdImageDestroy(out);

	return 0;
}


static void http_callback(int err, void *arg)
{
	struct avatar *avatar = arg;
	if (err) {
		http_ereply(avatar->conn, 500, "Error");
		goto out;
	}

	http_sreply(avatar->conn, 201, "Created", "text/html", "", 0, NULL);

out:
	mem_deref(avatar->conn);
	mem_deref((void *)avatar->msg);
	mem_deref(avatar);
}


int avatar_save(struct http_conn *conn, const struct http_msg *msg)
{
	struct avatar *avatar;

	if (!conn || !msg)
		return EINVAL;

	avatar = mem_zalloc(sizeof(struct avatar), NULL);
	if (!avatar)
		return ENOMEM;

	avatar->conn = mem_ref(conn);
	avatar->msg  = mem_ref((void *)msg);

	re_thread_async(work, http_callback, avatar);

	return 0;
}
