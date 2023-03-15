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


int slmix_db_sess_get(const struct pl *key, struct mbuf *data)
{
	int err, ret = 0;
	MDB_val mkey, mdata;
	MDB_txn *txn;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_get: mdb_txn_begin failed %m\n", err);
		return err;
	}

	err = mdb_get(txn, dbi_sess, &mkey, &mdata);
	if (err == MDB_NOTFOUND) {
		ret = ENODATA;
		goto out;
	}

	if (err) {
		warning("slmix_db_get: mdb_get failed %m\n", err);
		ret = err;
		goto out;
	}

	err = mbuf_write_mem(data, mdata.mv_data, mdata.mv_size);
	if (err) {
		ret = err;
		warning("slmix_db_get: mbuf_write_mem failed %m\n", err);
	}

out:
	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_get: mdb_txn_commit failed %m\n", err);
		return err;
	}

	return ret;
}


int slmix_db_sess_put(const struct pl *key, const struct pl *val)
{
	int err;
	MDB_val mkey, mdata;
	MDB_txn *txn;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	mdata.mv_data = (void *)val->p;
	mdata.mv_size = val->l;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_put: mdb_txn_begin failed %m\n", err);
		return err;
	}

	err = mdb_put(txn, dbi_sess, &mkey, &mdata, 0);
	if (err) {
		warning("slmix_db_put: mdb_put failed %m\n", err);
		return err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_put: mdb_txn_commit failed %m\n", err);
		return err;
	}

	return 0;
}


int slmix_db_sess_del(const struct pl *key)
{
	MDB_val mkey;
	MDB_txn *txn;
	int err;

	mkey.mv_data = (void *)key->p;
	mkey.mv_size = key->l;

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_del: mdb_txn_begin failed %m\n", err);
		return err;
	}

	err = mdb_del(txn, dbi_sess, &mkey, NULL);
	if (err) {
		warning("slmix_db_del: mdb_del failed %m\n", err);
		return err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_del: mdb_txn_commit failed %m\n", err);
		return err;
	}

	return err;
}


int slmix_db_init(void)
{
	int err;
	MDB_txn *txn;

	/* Before starting any other threads:
	- Create the environment.
	- Open a transaction.
	- Open all DBI handles the app will need.
	- Commit the transaction.
	- After that use the DBI handles freely among any transactions/threads.
	*/

	err = mdb_env_create(&env);
	if (err) {
		warning("slmix_db_init: mdb_env_create failed %m\n", err);
		return err;
	}

	/* Increase DB size to 32 MByte - must be a multiple of os page size */
	err = mdb_env_set_mapsize(env, 32 * 1024 * 1024);
	if (err) {
		warning("slmix_db_init: mdb_env_set_mapsize failed %m\n", err);
		return err;
	}

	err = mdb_env_set_maxdbs(env, 1);
	if (err) {
		warning("slmix_db_init: mdb_env_set_maxdbs failed %m\n", err);
		return err;
	}

	(void)fs_mkdir("database", 0700);

	err = mdb_env_open(env, "database", MDB_WRITEMAP | MDB_MAPASYNC, 0600);
	if (err) {
		warning("slmix_db_init: mdb_env_open failed %m\n", err);
		goto err;
	}

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_begin failed %m\n", err);
		goto err;
	}

	err = mdb_dbi_open(txn, "sessions", MDB_CREATE, &dbi_sess);
	if (err) {
		warning("slmix_db_init: mdb_dbi_open sess failed %m\n", err);
		goto err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_commit failed %m\n", err);
		mdb_dbi_close(env, dbi_sess);
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
	mdb_env_close(env);
}
