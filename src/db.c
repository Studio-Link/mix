#include <lmdb.h>
#include <mix.h>
#include <string.h>


/*
Basic LMDB concepts:
- MDB_env represents a database environment that can be used in
multiple processes. Created MDB_env object must be used by one process
only but in global picture all threads operate with the same
environment.

- MDB_dbi represents a DB which belongs to a environment. The same
environment can contain multiple named databases or an unnamed
database.

- MDB_txn represents a transaction. Multiple threads can open
transactions for the same MDB_env, but a particular Txn object must
only be accessed by one thread, and only one Txn object can be used on
a thread at a time. Note that only one write transaction can be open in
an environment in any given time. mdb_txn_begin() will simply block
until the previous one is either mdb_txn_commit() or mdb_txn_abort.

- MDB_cursor objects is used to iterate through data stored in DB or to
iterate over data for same key if DB supports multiple keys in same DB.
MDB_cursor is also used to insert or retrieve data.

- MDB_val object is used to store data for insertion or for fetching
data from DB.

- MDB_stat object have current status of DB.

- MDB_envinfo represents info received for current MDB_env object.
*/

static MDB_env *env = NULL;
static MDB_dbi dbi_sess;
static MDB_dbi dbi_rooms;


int slmix_db_up(unsigned int dbi)
{
	struct pl key;
	uint64_t time = tmr_jiffies_rt_usec() / 1000;

	pl_set_str(&key, "up");

	return slmix_db_put(dbi, &key, &time, sizeof(time));
}


int slmix_db_cur_open(void **cur, unsigned int dbi)
{
	int err;
	MDB_txn *txn = NULL;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_cursor_open: mdb_txn_begin failed %s\n",
			mdb_strerror(err));
		return err;
	}

	err = mdb_cursor_open(txn, dbi, (MDB_cursor **)cur);
	if (err) {
		warning("slmix_db_cursor_open: mdb_cursor_open failed %s\n",
			mdb_strerror(err));
		mdb_txn_abort(txn);
		return err;
	}

	return 0;
}


int slmix_db_cur_next(void *cur, struct mbuf *key, struct mbuf *data)
{
	MDB_val mkey, mdata;
	int err;

	err = mdb_cursor_get(cur, &mkey, &mdata, MDB_NEXT);
	if (err)
		return err;

	err = mbuf_write_mem(key, mkey.mv_data, mkey.mv_size);
	err |= mbuf_write_mem(data, mdata.mv_data, mdata.mv_size);
	if (err) {
		warning("slmix_db_cur_next: mbuf_write_mem failed %m\n", err);
		return err;
	}

	/* ensure 0-terminated safe key */
	if (key->buf[key->end - 1] != '\0')
		mbuf_write_u8(key, 0);

	/* ensure 0-terminated safe data */
	if (data->buf[data->end - 1] != '\0')
		mbuf_write_u8(data, 0);

	mbuf_set_pos(key, 0);
	mbuf_set_pos(data, 0);

	return 0;
}


int slmix_db_cur_close(void *cur)
{
	MDB_txn *txn = mdb_cursor_txn(cur);

	mdb_cursor_close(cur);
	mdb_txn_abort(txn);

	return 0;
}


int slmix_db_get(unsigned int dbi, const struct pl *key, struct mbuf *data)
{
	int err, ret = 0;
	MDB_val mkey, mdata;
	MDB_txn *txn = NULL;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_get: mdb_txn_begin failed %s\n",
			mdb_strerror(err));
		return err;
	}

	err = mdb_get(txn, dbi, &mkey, &mdata);
	if (err == MDB_NOTFOUND) {
		ret = ENODATA;
		goto out;
	}

	if (err) {
		warning("slmix_db_get: mdb_get failed %s\n",
			mdb_strerror(err));
		ret = err;
		goto out;
	}

	err = mbuf_write_mem(data, mdata.mv_data, mdata.mv_size);
	if (err) {
		ret = err;
		warning("slmix_db_get: mbuf_write_mem failed %m\n", err);
	}

	/* ensure 0-terminated safe string */
	if (data->buf[data->end - 1] != '\0')
		mbuf_write_u8(data, 0);

	mbuf_set_pos(data, 0);

out:
	mdb_txn_abort(txn);

	return ret;
}


int slmix_db_put(unsigned int dbi, const struct pl *key, void *val, size_t sz)
{
	int err;
	MDB_val mkey, mdata;
	MDB_txn *txn = NULL;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	mdata.mv_data = val;
	mdata.mv_size = sz;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_put: mdb_txn_begin failed %s\n",
			mdb_strerror(err));
		return err;
	}

	err = mdb_put(txn, dbi, &mkey, &mdata, 0);
	if (err) {
		warning("slmix_db_put: mdb_put failed %s\n",
			mdb_strerror(err));
		mdb_txn_abort(txn);
		return err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_put: mdb_txn_commit failed %s\n",
			mdb_strerror(err));
		return err;
	}

	return 0;
}


int slmix_db_del(unsigned int dbi, const struct pl *key)
{
	MDB_val mkey;
	MDB_txn *txn = NULL;
	int err;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_del: mdb_txn_begin failed %s\n",
			mdb_strerror(err));
		return err;
	}

	err = mdb_del(txn, dbi, &mkey, NULL);
	if (err) {
		warning("slmix_db_del: mdb_del failed %s\n",
			mdb_strerror(err));
		mdb_txn_abort(txn);
		return err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_del: mdb_txn_commit failed %s\n",
			mdb_strerror(err));
		return err;
	}

	return err;
}


unsigned int slmix_db_sess(void)
{
	return dbi_sess;
}


unsigned int slmix_db_rooms(void)
{
	return dbi_rooms;
}


int slmix_db_init(void)
{
	int err;
	MDB_txn *txn;
	char dbpath[PATH_SZ];

	/* Before starting any other threads:
	- Create the environment.
	- Open a transaction.
	- Open all DBI handles the app will need.
	- Commit the transaction.
	- After that use the DBI handles freely among any transactions/threads.
	*/

	re_snprintf(dbpath, sizeof(dbpath), "%s/database", slmix()->path);

	err = mdb_env_create(&env);
	if (err) {
		warning("slmix_db_init: mdb_env_create failed %s\n",
			mdb_strerror(err));
		return err;
	}

	/* Increase DB size to 32 MByte - must be a multiple of os page size */
	err = mdb_env_set_mapsize(env, 32 * 1024 * 1024);
	if (err) {
		warning("slmix_db_init: mdb_env_set_mapsize failed %s\n",
			mdb_strerror(err));
		return err;
	}

	err = mdb_env_set_maxdbs(env, 2);
	if (err) {
		warning("slmix_db_init: mdb_env_set_maxdbs failed %s\n",
			mdb_strerror(err));
		return err;
	}

	(void)fs_mkdir(dbpath, 0700);

	err = mdb_env_open(env, dbpath, MDB_WRITEMAP | MDB_MAPASYNC, 0600);
	if (err) {
		warning("slmix_db_init: mdb_env_open failed %s\n",
			mdb_strerror(err));
		goto err;
	}

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_begin failed %s\n",
			mdb_strerror(err));
		goto err;
	}

	err = mdb_dbi_open(txn, "sessions", MDB_CREATE, &dbi_sess);
	if (err) {
		warning("slmix_db_init: mdb_dbi_open sess failed %s\n",
			mdb_strerror(err));
		goto err;
	}

	err = mdb_dbi_open(txn, "rooms", MDB_CREATE, &dbi_rooms);
	if (err) {
		warning("slmix_db_init: mdb_dbi_open rooms failed %s\n",
			mdb_strerror(err));
		goto err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_commit failed %s\n",
			mdb_strerror(err));
		mdb_dbi_close(env, dbi_sess);
		mdb_dbi_close(env, dbi_rooms);
		goto err;
	}

	return 0;

err:
	mdb_env_close(env);
	return err;
}


void slmix_db_close(void)
{
	mdb_dbi_close(env, dbi_sess);
	mdb_dbi_close(env, dbi_rooms);
	mdb_env_close(env);
}
