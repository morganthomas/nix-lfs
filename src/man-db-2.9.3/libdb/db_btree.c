/*
 * db_btree.c: low level btree interface routines for man.
 *
 * Copyright (C) 1994, 1995 Graeme W. Wilford. (Wilf.)
 * Copyright (C) 2002 Colin Watson.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Mon Aug  8 20:35:30 BST 1994  Wilf. (G.Wilford@ee.surrey.ac.uk)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

/* below this line are routines only useful for the BTREE interface */
#ifdef BTREE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/file.h> /* for flock() */
#include <sys/types.h> /* for open() */
#include <sys/stat.h>

#include "gl_hash_set.h"
#include "gl_xset.h"
#include "stat-time.h"
#include "timespec.h"

#include "manconfig.h"

#include "error.h"
#include "glcontainers.h"

#include "mydbm.h"
#include "db_storage.h"

gl_set_t loop_check;

/* the Berkeley database libraries do nothing to arbitrate between concurrent
   database accesses, so we do a simple flock(). If the db is opened in
   anything but O_RDONLY mode, an exclusive lock is enabled. Otherwise, the
   lock is shared. A file cannot have both locks at once, and the non
   blocking method is used ": Try again". This adopts GNU dbm's approach. */

/* release the lock and close the database */
void man_btree_close (man_btree_wrapper wrap)
{
	if (!wrap)
		return;

	free (wrap->name);
	(void) flock ((wrap->file->fd) (wrap->file), LOCK_UN);
	(wrap->file->close) (wrap->file);
	free (wrap);
}

/* open a btree type database, with file locking. */
man_btree_wrapper man_btree_open (const char *name, int flags, int mode)
{
	man_btree_wrapper wrap;
	DB *file;
	BTREEINFO b;
	int lock_op;
	int lock_failed;

	b.flags = 0;		/* do not allow any duplicate keys */

	b.cachesize = 0;	/* default size */
	b.maxkeypage = 0;	/* default */
	b.minkeypage = 0;	/* default */
	b.psize = 0;		/* default page size (2048?) */
	b.compare = NULL;	/* builtin compare() function */
	b.prefix = NULL;	/* builtin function */
	b.lorder = 0;		/* byte order of host */

	if (flags & ~O_RDONLY) {
		/* flags includes O_RDWR or O_WRONLY, need an exclusive lock */
		lock_op = LOCK_EX | LOCK_NB;
	} else {
		lock_op = LOCK_SH | LOCK_NB;
	}

	if (!(flags & O_CREAT)) {
		/* Berkeley DB thinks that a zero-length file means that
		 * somebody else is writing it, and sleeps for a few
		 * seconds to give the writer a chance. All very well, but
		 * the common case is that the database is just zero-length
		 * because mandb was interrupted or ran out of disk space or
		 * something like that - so we check for this case by hand
		 * and ignore the database if it's zero-length.
		 */
		struct stat iszero;
		if (stat (name, &iszero) < 0)
			return NULL;
		if (iszero.st_size == 0) {
			errno = EINVAL;
			return NULL;
		}
	}

	if (flags & O_TRUNC) {
		/* opening the db is destructive, need to lock first */
		int fd;

		file = NULL;
		lock_failed = 1;
		fd = open (name, flags & ~O_TRUNC, mode);
		if (fd != -1) {
			if (!(lock_failed = flock (fd, lock_op)))
				file = dbopen (name, flags, mode,
					       DB_BTREE, &b);
			close (fd);
		}
	} else {
		file = dbopen (name, flags, mode, DB_BTREE, &b);
		if (file)
			lock_failed = flock ((file->fd) (file), lock_op);
	}

	if (!file)
		return NULL;

	if (lock_failed) {
		gripe_lock (name);
		(file->close) (file);
		return NULL;
	}

	wrap = xmalloc (sizeof *wrap);
	wrap->name = xstrdup (name);
	wrap->file = file;

	return wrap;
}

/* do a replace when we have the duplicate flag set on the database -
   we must do a del and insert, as a direct insert will not wipe out the
   old entry */
int man_btree_replace (man_btree_wrapper wrap, datum key, datum cont)
{
	return (wrap->file->put) (wrap->file, (DBT *) &key, (DBT *) &cont, 0);
}

int man_btree_insert (man_btree_wrapper wrap, datum key, datum cont)
{
	return (wrap->file->put) (wrap->file, (DBT *) &key, (DBT *) &cont,
				  R_NOOVERWRITE);
}

/* generic fetch routine for the btree database */
datum man_btree_fetch (man_btree_wrapper wrap, datum key)
{
	datum data;

	memset (&data, 0, sizeof data);

	if ((wrap->file->get) (wrap->file, (DBT *) &key, (DBT *) &data, 0)) {
		memset (&data, 0, sizeof data);
		return data;
	}

	return copy_datum (data);
}

/* return 1 if the key exists, 0 otherwise */
int man_btree_exists (man_btree_wrapper wrap, datum key)
{
	datum data;
	return ((wrap->file->get) (wrap->file, (DBT *) &key, (DBT *) &data,
				   0) ? 0 : 1);
}

/* initiate a sequential access */
static datum man_btree_findkey (man_btree_wrapper wrap, u_int flags)
{
	datum key, data;
	char *loop_check_key;

	memset (&key, 0, sizeof key);
	memset (&data, 0, sizeof data);

	if (flags == R_FIRST) {
		if (loop_check) {
			gl_set_free (loop_check);
			loop_check = NULL;
		}
	}
	if (!loop_check)
		loop_check = new_string_set (GL_HASH_SET);

	if (((wrap->file->seq) (wrap->file, (DBT *) &key, (DBT *) &data,
				flags))) {
		memset (&key, 0, sizeof key);
		return key;
	}

	loop_check_key = xstrndup (MYDBM_DPTR (key), MYDBM_DSIZE (key));
	if (gl_set_search (loop_check, loop_check_key)) {
		/* We've seen this key already, which is broken. Return NULL
		 * so the caller doesn't go round in circles.
		 */
		debug ("Corrupt database! Already seen %*s. "
		       "Attempting to recover ...\n",
		       (int) MYDBM_DSIZE (key), MYDBM_DPTR (key));
		memset (&key, 0, sizeof key);
		free (loop_check_key);
		return key;
	}

	gl_set_add (loop_check, loop_check_key);

	return copy_datum (key);
}

/* return the first key in the db */
datum man_btree_firstkey (man_btree_wrapper wrap)
{
	return man_btree_findkey (wrap, R_FIRST);
}

/* return the next key in the db. NB. This routine only works if the cursor
   has been previously set by man_btree_firstkey() since it was last opened. So
   if we close/reopen a db mid search, we have to manually set up the
   cursor again. */
datum man_btree_nextkey (man_btree_wrapper wrap)
{
	return man_btree_findkey (wrap, R_NEXT);
}

/* compound nextkey routine, initialising key and content */
int man_btree_nextkeydata (man_btree_wrapper wrap, datum *key, datum *cont)
{
	int status;

	if ((status = (wrap->file->seq) (wrap->file, (DBT *) key, (DBT *) cont,
					 R_NEXT)) != 0)
		return status;

	*key = copy_datum (*key);
	*cont = copy_datum (*cont);

	return 0;
}

struct timespec man_btree_get_time (man_btree_wrapper wrap)
{
	struct stat st;

	if (fstat ((wrap->file->fd) (wrap->file), &st) < 0) {
		struct timespec t;
		t.tv_sec = -1;
		t.tv_nsec = -1;
		return t;
	}
	return get_stat_mtime (&st);
}

void man_btree_set_time (man_btree_wrapper wrap, const struct timespec time)
{
	struct timespec times[2];

	times[0] = time;
	times[1] = time;
	futimens ((wrap->file->fd) (wrap->file), times);
}

#endif /* BTREE */
