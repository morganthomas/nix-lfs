/*
 * man-recode.c: convert manual pages to another encoding
 *
 * Copyright (C) 2019 Colin Watson.
 *
 * This file is part of man-db.
 *
 * man-db is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * man-db is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with man-db; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "argp.h"
#include "dirname.h"
#include "gl_array_list.h"
#include "gl_xlist.h"
#include "progname.h"
#include "tempname.h"

#include "gettext.h"
#define _(String) gettext (String)
#define N_(String) gettext_noop (String)

#include "manconfig.h"

#include "pipeline.h"

#include "cleanup.h"
#include "encodings.h"
#include "error.h"
#include "glcontainers.h"
#include "sandbox.h"

#include "decompress.h"
#include "manconv_client.h"

int quiet = 0;
man_sandbox *sandbox;

static char *to_code;
static gl_list_t filenames;
static const char *suffix;
static bool in_place;

struct try_file_at_args {
	int dir_fd;
	int flags;
};

static int
try_file_at (char *tmpl, void *flags)
{
	struct try_file_at_args *args = flags;
	return openat (args->dir_fd, tmpl,
		       (args->flags & ~O_ACCMODE) | O_RDWR | O_CREAT | O_EXCL,
		       S_IRUSR | S_IWUSR);
}

static int
mkstempat (int dir_fd, char *xtemplate)
{
	struct try_file_at_args args;

	args.dir_fd = dir_fd;
	args.flags = 0;
	return try_tempname (xtemplate, 0, &args, try_file_at);
}

enum opts {
	OPT_SUFFIX = 256,
	OPT_IN_PLACE = 257,
	OPT_MAX
};

const char *argp_program_version = "man-recode " PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;
error_t argp_err_exit_status = FAIL;

static const char args_doc[] =
	N_("-t CODE {--suffix SUFFIX | --in-place} FILENAME...");

static struct argp_option options[] = {
	{ "to-code",	't',	N_("CODE"),	0,	N_("encoding for output") },
	{ "suffix",	OPT_SUFFIX,
				N_("SUFFIX"),	0,	N_("suffix to append to output file name") },
	{ "in-place",	OPT_IN_PLACE,
				0,		0,	N_("overwrite input files in place") },
	{ "debug",	'd',	0,		0,	N_("emit debugging messages") },
	{ "quiet",	'q',	0,		0,	N_("produce fewer warnings") },
	{ 0, 'h', 0, OPTION_HIDDEN, 0 }, /* compatibility for --help */
	{ 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	switch (key) {
		case 't':
			to_code = xstrdup (arg);
			return 0;
		case OPT_SUFFIX:
			suffix = arg;
			return 0;
		case OPT_IN_PLACE:
			in_place = true;
			return 0;
		case 'd':
			debug_level = true;
			return 0;
		case 'q':
			quiet = 1;
			return 0;
		case 'h':
			argp_state_help (state, state->out_stream,
					 ARGP_HELP_STD_HELP);
			break;
		case ARGP_KEY_ARG:
			gl_list_add_last (filenames, xstrdup (arg));
			return 0;
		case ARGP_KEY_NO_ARGS:
			argp_usage (state);
			break;
		case ARGP_KEY_SUCCESS:
			if (!to_code)
				argp_error (state,
					    _("must specify an output "
					      "encoding"));
			if (!suffix && !in_place)
				argp_error (state,
					    _("must use either --suffix or "
					      "--in-place"));
			if (suffix && in_place)
				argp_error (state,
					    _("--suffix and --in-place are "
					      "mutually exclusive"));
			return 0;
	}
	return ARGP_ERR_UNKNOWN;
}

static struct argp argp = { options, parse_opt, args_doc };

static void recode (const char *filename)
{
	pipeline *decomp, *convert;
	struct compression *comp;
	int dir_fd = -1;
	char *dirname, *basename, *stem, *outfilename;
	char *page_encoding;
	int status;

	decomp = decompress_open (filename);
	if (!decomp)
		error (FAIL, 0, _("can't open %s"), filename);

	dirname = dir_name (filename);
	basename = base_name (filename);
	comp = comp_info (basename, 1);
	if (comp)
		stem = comp->stem;	/* steal memory */
	else
		stem = xstrdup (basename);

	convert = pipeline_new ();
	if (suffix) {
		outfilename = xasprintf ("%s/%s%s", dirname, stem, suffix);
		pipeline_want_outfile (convert, outfilename);
	} else {
		int dir_fd_open_flags;
		char *template_path;
		int outfd;

		dir_fd_open_flags = O_SEARCH | O_DIRECTORY;
#ifdef O_PATH
		dir_fd_open_flags |= O_PATH;
#endif
		dir_fd = open (dirname, dir_fd_open_flags);
		if (dir_fd < 0)
			error (FATAL, errno, _("can't open %s"), dirname);

		outfilename = xasprintf ("%s.XXXXXX", stem);
		/* For error messages. */
		template_path = xasprintf ("%s/%s", dirname, outfilename);
		outfd = mkstempat (dir_fd, outfilename);
		if (outfd == -1) {
			error (FATAL, errno,
			       _("can't open temporary file %s"),
			       template_path);
		}
		free (template_path);
		pipeline_want_out (convert, outfd);
	}

	pipeline_start (decomp);
	page_encoding = check_preprocessor_encoding (decomp, NULL, NULL);
	if (!page_encoding) {
		char *lang = lang_dir (filename);
		page_encoding = get_page_encoding (lang);
		free (lang);
	}
	debug ("guessed input encoding %s for %s\n", page_encoding, filename);
	add_manconv (convert, page_encoding, to_code);

	if (!pipeline_get_ncommands (convert))
		pipeline_command (convert, pipecmd_new_passthrough ());

	pipeline_connect (decomp, convert, (void *) 0);
	pipeline_pump (decomp, convert, (void *) 0);
	pipeline_wait (decomp);
	status = pipeline_wait (convert);
	if (status != 0)
		error (CHILD_FAIL, 0, _("command exited with status %d: %s"),
		       status, pipeline_tostring (convert));

	if (in_place) {
		assert (dir_fd != -1);
		if (renameat (dir_fd, outfilename, dir_fd, stem) == -1) {
			char *outfilepath = xasprintf
				("%s/%s", dirname, outfilename);
			unlink (outfilename);
			error (FATAL, errno, _("can't rename %s to %s"),
			       outfilepath, filename);
		}
		debug ("stem: %s, basename: %s\n", stem, basename);
		if (!STREQ (stem, basename)) {
			if (unlinkat (dir_fd, basename, 0) == -1)
				error (FATAL, errno, _("can't remove %s"),
				       filename);
		}
	}

	free (page_encoding);
	free (outfilename);
	free (stem);
	free (basename);
	free (dirname);
	if (dir_fd)
		close (dir_fd);
	pipeline_free (convert);
	pipeline_free (decomp);
}

int main (int argc, char *argv[])
{
	const char *filename;

	set_program_name (argv[0]);

	init_debug ();
	pipeline_install_post_fork (pop_all_cleanups);
	sandbox = sandbox_init ();
	init_locale ();
	filenames = new_string_list (GL_ARRAY_LIST, true);

	if (argp_parse (&argp, argc, argv, 0, 0, 0))
		exit (FAIL);

	GL_LIST_FOREACH_START (filenames, filename)
		recode (filename);
	GL_LIST_FOREACH_END (filenames);

	free (to_code);

	gl_list_free (filenames);
	sandbox_free (sandbox);

	return 0;
}
