#include <gd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mix.h>

struct avatar {
	struct http_conn *conn;
	struct mbuf *mb;
	struct session *sess;
};


static int work(void *arg)
{
	struct avatar *avatar = arg;
	struct mbuf *mb	      = avatar->mb;
	FILE *img;
	int w = 256;
	int h = 256;
	gdImagePtr in, out;
	struct pl pl  = PL_INIT;
	int err	      = 0;
	size_t offset = 0x17; /* "data:image/png;base64, */
	char file[PATH_SZ];

	if (mbuf_get_left(mb) < offset + 1) /* offset + ending '"' */
		return EINVAL;

	pl.l = mbuf_get_left(mb) * 2;
	pl.p = mem_zalloc(pl.l, NULL);
	if (!pl.p)
		return ENOMEM;

	mbuf_advance(mb, offset);

	err = base64_decode((char *)mbuf_buf(mb), mbuf_get_left(mb) - 1,
			    (uint8_t *)pl.p, &pl.l);
	if (err) {
		mem_deref((void *)pl.p);
		warning("avatar: base64_decode failed %m\n", err);
		return err;
	}

	in = gdImageCreateFromPngPtr((int)pl.l, (void *)pl.p);
	if (!in) {
		warning("\navatar: create image failed\n");
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

	debug("write %s/webui/public/avatars/%s.[png,webp]\n", slmix()->path,
	      avatar->sess->user->id);

	/* PNG */
	re_snprintf(file, sizeof(file), "%s/webui/public/avatars/%s.png",
		    slmix()->path, avatar->sess->user->id);

	err = fs_fopen(&img, file, "w+");
	if (err) {
		warning("avatar: write png failed %m\n", err);
		goto out;
	}

	out->saveAlphaFlag = true;
	gdImagePng(out, img);
	fclose(img);
	chmod(file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	/* WEBP */
	re_snprintf(file, sizeof(file), "%s/webui/public/avatars/%s.webp",
		    slmix()->path, avatar->sess->user->id);
	err = fs_fopen(&img, file, "w+");
	if (err) {
		warning("avatar: write webp failed %m\n", err);
		goto out;
	}

	out->saveAlphaFlag = true;
	gdImageWebp(out, img);
	fclose(img);
	chmod(file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
	char *json	      = NULL;

	if (err) {
		http_ereply(avatar->conn, 500, "Error");
		goto out;
	}

	err = user_event_json(&json, USER_ADDED, avatar->sess);
	if (err) {
		http_ereply(avatar->conn, 500, "Error");
		goto out;
	}

	http_sreply(avatar->conn, 201, "Created", "text/html", json,
		    str_len(json), avatar->sess);

out:
	mem_deref(json);
	mem_deref(avatar);
}


static void avatar_destruct(void *data)
{
	struct avatar *avatar = data;
	mem_deref(avatar->conn);
	mem_deref(avatar->mb);
	mem_deref(avatar->sess);
}


int avatar_save(struct session *sess, struct http_conn *conn,
		const struct http_msg *msg)
{
	struct avatar *avatar;

	if (!sess || !conn || !msg)
		return EINVAL;

	avatar = mem_zalloc(sizeof(struct avatar), avatar_destruct);
	if (!avatar)
		return ENOMEM;

	avatar->conn = mem_ref(conn);
	avatar->sess = mem_ref(sess);
	avatar->mb   = mem_ref(msg->mb);

	re_thread_async(work, http_callback, avatar);

	return 0;
}


int avatar_delete(struct session *sess)
{
	char file[PATH_SZ];
	int err;

	re_snprintf(file, sizeof(file), "%s/webui/public/avatars/%s.png",
		    slmix()->path, sess->user->id);
	err = unlink(file);
	if (err)
		return errno;

	re_snprintf(file, sizeof(file), "%s/webui/public/avatars/%s.webp",
		    slmix()->path, sess->user->id);
	err = unlink(file);
	if (err)
		return errno;

	return 0;
}
