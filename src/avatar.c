#include <re.h>
#include <baresip.h>
#include <gd.h>

#include "mix.h"

struct avatar {
	struct http_conn *conn;
	struct mbuf *mb;
};


static int work(void *arg)
{
	struct avatar *avatar = arg;
	struct mbuf *mb	      = avatar->mb;
	FILE *jpg;
	int w = 256;
	int h = 256;
	gdImagePtr in, out;
	struct pl pl = PL_INIT;
	int err	     = 0;

	pl.l = mbuf_get_left(mb) * 2;
	pl.p = mem_zalloc(pl.l, NULL);
	if (!pl.p)
		return ENOMEM;

	mbuf_advance(mb, 0x18); /* "data:image/jpeg;base64, */

	err = base64_decode((char *)mbuf_buf(mb), mbuf_get_left(mb) - 1,
		      (uint8_t *)pl.p, &pl.l);
	if (err) {
		mem_deref((void *)pl.p);
		warning("avatar: base64_decode failed %m\n", err);
		return err;
	}

	in = gdImageCreateFromJpegPtr((int)pl.l, (void *)pl.p);
	if (!in) {
		warning("avatar: create image failed\n");
		err = EIO;
		goto err;
	}

	gdImageSetInterpolationMethod(in, GD_BILINEAR_FIXED);
	out = gdImageScale(in, w, h);
	if (!out) {
		gdImageDestroy(in);
		warning("avatar: image scale failed\n");
		err = EIO;
		goto err;
	}

	err = fs_fopen(&jpg, "/tmp/test.jpg", "w+");
	if (err) {
		warning("avatar: write open failed %m\n", err);
		goto out;
	}
	gdImageJpeg(out, jpg, 90);
	fclose(jpg);

out:
	gdImageDestroy(in);
	gdImageDestroy(out);

err:
	mem_deref((void *)pl.p);

	return err;
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
	mem_deref(avatar);
}


static void avatar_destruct(void *data)
{
	struct avatar *avatar = data;
	mem_deref(avatar->conn);
	mem_deref(avatar->mb);
}


int avatar_save(struct http_conn *conn, const struct http_msg *msg)
{
	struct avatar *avatar;

	if (!conn || !msg)
		return EINVAL;

	avatar = mem_zalloc(sizeof(struct avatar), avatar_destruct);
	if (!avatar)
		return ENOMEM;

	avatar->conn = mem_ref(conn);
	avatar->mb   = mem_ref(msg->mb);

	/* slow fs operations and image scaling */
	re_thread_async(work, http_callback, avatar);

	return 0;
}
