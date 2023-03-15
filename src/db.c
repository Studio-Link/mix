#include <lmdb.h>
#include <mix.h>


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
static MDB_dbi dbi;


int slmix_db_get(struct pl *key, struct pl *val)
{
	(void)key;
	(void)val;

	return 0;
}


int slmix_db_put(struct pl *key, struct pl *val)
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
		warning("slmix_db_set: mdb_txn_begin failed %m\n", err);
		return err;
	}

	err = mdb_put(txn, dbi, &mkey, &mdata, 0);
	if (err) {
		warning("slmix_db_set: mdb_put failed %m\n", err);
		return err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_set: mdb_txn_commit failed %m\n", err);
		return err;
	}

	return 0;
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

	(void)fs_mkdir("/tmp/sl", 0700);

	err = mdb_env_open(env, "/tmp/sl", 0, 0600);
	if (err) {
		warning("slmix_db_init: mdb_env_open failed %m\n", err);
		goto err;
	}

	err = mdb_txn_begin(env, NULL, 0, &txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_begin failed %m\n", err);
		goto err;
	}

	err = mdb_dbi_open(txn, NULL, 0, &dbi);
	if (err) {
		warning("slmix_db_init: mdb_dbi_open failed %m\n", err);
		goto err;
	}

	err = mdb_txn_commit(txn);
	if (err) {
		warning("slmix_db_init: mdb_txn_commit failed %m\n", err);
		mdb_dbi_close(env, dbi);
		goto err;
	}

	return 0;

err:
	mdb_env_close(env);
	return err;
}


void slmix_db_close(void)
{
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
}
