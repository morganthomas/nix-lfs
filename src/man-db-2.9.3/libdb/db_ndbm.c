/*
 * db_ndbm.c: low level ndbm interface routines for man.
 *
 * Copyright (C) 1994, 1995 Graeme W. Wilford. (Wilf.)
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
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef NDBM

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/file.h> /* for flock() */
#include <sys/types.h> /* for open() */
#include <sys/stat.h>

#include "stat-time.h"
#include "timespec.h"
#include "xvasprintf.h"

#include "manconfig.h"

#include "db_storage.h"
#include "db_xdbm.h"
#include "mydbm.h"

/* release the lock and close the database */
static void raw_close (man_ndbm_wrapper wrap)
{
	flock (dbm_dirfno (wrap->file), LOCK_UN);
	dbm_close (wrap->file);
}

void man_ndbm_close (man_ndbm_wrapper wrap)
{
	man_xdbm_close (wrap, raw_close);
}

/* open a ndbm type database, with file locking. */
man_ndbm_wrapper man_ndbm_open (const char *name, int flags, int mode)
{
	man_ndbm_wrapper wrap;
	char *name_copy;
	DBM *file;
	int lock_op;
	int lock_failed;

	if (flags & ~O_RDONLY) {
		/* flags includes O_RDWR or O_WRONLY, need an exclusive lock */
		lock_op = LOCK_EX | LOCK_NB;
	} else {
		lock_op = LOCK_SH | LOCK_NB;
	}

	/* At least GDBM's version of dbm_open declares the file name
	 * parameter as non-const.  This is probably incorrect, but take a
	 * copy just in case.
	 */
	name_copy = xstrdup (name);

	if (flags & O_TRUNC) {
		/* opening the db is destructive, need to lock first */
		char *dir_fname;
		int dir_fd;

		file = NULL;
		lock_failed = 1;
		dir_fname = xasprintf ("%s.dir", name);
		dir_fd = open (dir_fname, flags & ~O_TRUNC, mode);
		free (dir_fname);
		if (dir_fd != -1) {
			if (!(lock_failed = flock (dir_fd, lock_op)))
				file = dbm_open (name_copy, flags, mode);
			close (dir_fd);
		}
	} else {
		file = dbm_open (name_copy, flags, mode);
		if (file)
			lock_failed = flock (dbm_dirfno (file), lock_op);
	}

	free (name_copy);

	if (!file)
		return NULL;

	if (lock_failed) {
		gripe_lock (name);
		dbm_close (file);
		return NULL;
	}

	wrap = xmalloc (sizeof *wrap);
	wrap->name = xstrdup (name);
	wrap->file = file;

	return wrap;
}

static datum unsorted_firstkey (man_ndbm_wrapper wrap)
{
	return copy_datum (dbm_firstkey (wrap->file));
}

static datum unsorted_nextkey (man_ndbm_wrapper wrap, datum key _GL_UNUSED)
{
	return copy_datum (dbm_nextkey (wrap->file));
}

datum man_ndbm_firstkey (man_ndbm_wrapper wrap)
{
	return man_xdbm_firstkey (wrap, unsorted_firstkey, unsorted_nextkey);
}

datum man_ndbm_nextkey (man_ndbm_wrapper wrap, datum key)
{
	return man_xdbm_nextkey (wrap, key);
}

struct timespec man_ndbm_get_time (man_ndbm_wrapper wrap)
{
	struct stat st;

	if (fstat (dbm_dirfno (wrap->file), &st) < 0) {
		struct timespec t;
		t.tv_sec = -1;
		t.tv_nsec = -1;
		return t;
	}
	return get_stat_mtime (&st);
}

void man_ndbm_set_time (man_ndbm_wrapper wrap, const struct timespec time)
{
	struct timespec times[2];

	times[0] = time;
	times[1] = time;
	futimens (dbm_dirfno (wrap->file), times);
}

#endif /* NDBM */
