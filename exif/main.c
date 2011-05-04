/* main.c
 *
 * Copyright © 2002 Lutz Müller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-loader.h>

#include "actions.h"
#include "exif-i18n.h"
#include "utils.h"

/* Must be loaded after exif-i18n.h */
#include <popt.h>

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

#ifdef ENABLE_GLIBC_MEMDEBUG
#include <mcheck.h>
#endif

/*! The minimum width of output that we can support */
#define MIN_WIDTH 52

/*! A sane limit on output width */
#define MAX_WIDTH 9999

/* Old versions of popt.h don't define POPT_TABLEEND */
#ifndef POPT_TABLEEND
#  define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif

/* ANSI escape codes for output colors */
#define COL_BLUE        "\033[34m"
#define COL_GREEN       "\033[32m"
#define COL_RED         "\033[31m"
#define COL_BOLD        "\033[1m"
#define COL_UNDERLINE   "\033[4m"
#define COL_NORMAL      "\033[m"

/* Ensure that we're actually printing the escape codes to a TTY.
 * FIXME: We should make sure the terminal can actually understand these
 *        escape sequences, or better yet, use vidattr(3X) to set colours
 *        instead.
 */

#if defined(HAVE_ISATTY) && defined(HAVE_FILENO)
#  define file_is_tty(file) (isatty(fileno(file)))
#else
#  define file_is_tty(file) (0==1)
#endif

#define put_colorstring(file, colorstring) \
	do { \
		if (file_is_tty(file)) { \
			fputs (colorstring, file); \
		} \
	} while (0)

typedef struct {
	unsigned int debug;

       /* input */
       int ignore_corrupted;
       /* output */
       int corrupted;
} LogArg;

static void
log_func (ExifLog *log, ExifLogCode code, const char *domain,
	  const char *format, va_list args, void *data)
{
	LogArg *log_arg = data;

	log_arg->corrupted = 0;

	/*
	 * When debugging, continue as far as possible. If not, make all errors
	 * fatal.
	 */
	switch (code) {
	case -1:
		put_colorstring (stderr, COL_RED);
		vfprintf (stderr, format, args);
		fprintf (stderr, "\n");
		put_colorstring (stderr, COL_NORMAL);
		exit (1);
	case EXIF_LOG_CODE_DEBUG:
		if (log_arg->debug) {
			put_colorstring (stdout, COL_GREEN);
			fprintf (stdout, "%s: ", domain);
			vfprintf (stdout, format, args);
			put_colorstring (stdout, COL_NORMAL);
			printf ("\n");
		}
		break;
	case EXIF_LOG_CODE_CORRUPT_DATA:
		log_arg->corrupted = 1;
		/* We ignore corrupted data event in some cases */
		if (log_arg->ignore_corrupted)
			return;
		/* Fall through to EXIF_LOG_CODE_NO_MEMORY */
	case EXIF_LOG_CODE_NO_MEMORY:
		put_colorstring (stderr, COL_RED COL_BOLD COL_UNDERLINE);
		fprintf (stderr, "%s\n", exif_log_code_get_title (code));
		put_colorstring (stderr, COL_NORMAL COL_RED);
		fprintf (stderr, "%s\n", exif_log_code_get_message (code));
		fprintf (stderr, "%s: ", domain);
		vfprintf (stderr, format, args);
		put_colorstring (stderr, COL_NORMAL);
		fprintf (stderr, "\n");

		/*
		 * EXIF_LOG_CODE_NO_MEMORY is always a fatal error, so exit. 
		 * EXIF_LOG_CODE_CORRUPT_DATA is only fatal if debug mode
		 * is off.
		 *
		 * Exiting the program due to a log message is really a bad
		 * idea to begin with. This should be removed once the libexif
		 * API is fixed to properly return error codes everywhere.
		 */
		if ((code == EXIF_LOG_CODE_NO_MEMORY) || !log_arg->debug)
			exit (1);
		break;
	default:
		if (log_arg->debug) {
			put_colorstring (stdout, COL_BLUE);
			printf ("%s: ", domain);
			vprintf (format, args);
			put_colorstring (stdout, COL_NORMAL);
			printf ("\n");
		}
		break;
	}
}

/*
 * Static variables. I had them first in main (), but people
 * compiling exif on IRIX complained about that not being compatible
 * with the "SGI MIPSpro C compiler", and elsewhere on Open Watcom C.
 * This is due to the inability of these compilers to initialize a
 * struct with the address of a stack-allocated variable.
 */
static unsigned int list_tags = 0, show_description = 0;
static unsigned int xml_output = 0;
static unsigned int extract_thumbnail = 0, remove_thumb = 0;
static unsigned int remove_tag = 0, create_exif = 0, no_fixup = 0;
static unsigned int list_mnote = 0;
static unsigned int show_version = 0;
static char *output = NULL;
static char *ifd_string = NULL, *tag_string = NULL;
static ExifParams p = {EXIF_INVALID_TAG, EXIF_IFD_COUNT, 0, 0, 80,
		      		   NULL, NULL,NULL};
LogArg log_arg = {0, 0, 0};

int
main (int argc, const char **argv)
{
	/* POPT_ARG_NONE needs an int, not char! */
	poptContext ctx;
	const char * const *args;
	const struct poptOption options[] = {
		POPT_AUTOHELP
		{"version", 'v', POPT_ARG_NONE, &show_version, 0,
		 N_("Display software version"), NULL},
		{"ids", 'i', POPT_ARG_NONE, &p.use_ids, 0,
		 N_("Show IDs instead of tag names"), NULL},
		{"tag", 't', POPT_ARG_STRING, &tag_string, 0,
		 N_("Select tag"), N_("tag")},
		{"ifd", '\0', POPT_ARG_STRING, &ifd_string, 0,
		 N_("Select IFD"), N_("IFD")},
		{"list-tags", 'l', POPT_ARG_NONE, &list_tags, 0,
		 N_("List all EXIF tags"), NULL},
		{"show-mnote", '|', POPT_ARG_NONE, &list_mnote, 0,
		 N_("Show contents of tag MakerNote"), NULL},
		{"remove", '\0', POPT_ARG_NONE, &remove_tag, 0,
		 N_("Remove tag or ifd"), NULL},
		{"show-description", 's', POPT_ARG_NONE, &show_description, 0,
		 N_("Show description of tag"), NULL},
		{"extract-thumbnail", 'e', POPT_ARG_NONE, &extract_thumbnail, 0,
		 N_("Extract thumbnail"), NULL},
		{"remove-thumbnail", 'r', POPT_ARG_NONE, &remove_thumb, 0,
		 N_("Remove thumbnail"), NULL},
		{"insert-thumbnail", 'n', POPT_ARG_STRING, &p.set_thumb, 0,
		 N_("Insert FILE as thumbnail"), N_("FILE")},
		{"no-fixup", '\0', POPT_ARG_NONE, &no_fixup, 0,
		 N_("Do not fix existing tags in files"), NULL},
		{"output", 'o', POPT_ARG_STRING, &output, 0,
		 N_("Write data to FILE"), N_("FILE")},
		{"set-value", '\0', POPT_ARG_STRING, &p.set_value, 0,
		 N_("Value of tag"), N_("STRING")},
		{"create-exif", 'c', POPT_ARG_NONE, &create_exif, 0,
		 N_("Create EXIF data if not existing"), NULL},
		{"machine-readable", 'm', POPT_ARG_NONE, &p.machine_readable, 0,
		 N_("Output in a machine-readable (tab delimited) format"),
		 NULL},
		{"width", 'w', POPT_ARG_INT, &p.width, 0,
		 N_("Width of output"), N_("WIDTH")},
		{"xml-output", 'x', POPT_ARG_NONE, &xml_output, 0,
		 N_("Output in a XML format"),
		 NULL},
		{"debug", 'd', POPT_ARG_NONE, &log_arg.debug, 0,
		 N_("Show debugging messages"), NULL},
		POPT_TABLEEND};
#if 0
/* This is a hack to allow translation of popt 1.10 messages with gettext.
 * Supposedly, this won't be necessary starting with popt 1.12
 */
		N_("Help options:");
		N_("Show this help message");
		N_("Display brief usage message");
#endif
	ExifData *ed;
	ExifLog *log = NULL;
	char fout[1024] = {0, };
	int continue_without_file = 0;

#ifdef ENABLE_GLIBC_MEMDEBUG
	mcheck (NULL);
	mtrace ();
#endif

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
	setlocale (LC_ALL, "");
#endif
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);
	poptSetOtherOptionHelp (ctx, _("[OPTION...] file"));
	while (poptGetNextOpt (ctx) > 0)
		;

	p.width = MIN(MAX_WIDTH, MAX(MIN_WIDTH, p.width));

	log = exif_log_new ();
	exif_log_set_func (log, log_func, &log_arg);

	/* Identify the parameters */
	if (ifd_string) {
		p.ifd = exif_ifd_from_string (ifd_string);
		if ((p.ifd < EXIF_IFD_0) || (p.ifd >= EXIF_IFD_COUNT) ||
		    !exif_ifd_get_name (p.ifd)) {
			exif_log (log, -1, "exif",
				_("Invalid IFD '%s'. Valid IFDs are "
				"'0', '1', 'EXIF', 'GPS', and "
				"'Interoperability'."), ifd_string);
			return 1;
		}
	}
	if (tag_string) {
		p.tag = exif_tag_from_string (tag_string);
		if (p.tag == EXIF_INVALID_TAG) {
			exif_log (log, -1, "exif", _("Invalid tag '%s'!"),
				tag_string);
			return 1;
		}
	}

	/* Check for all necessary parameters */
	if ((p.tag == EXIF_INVALID_TAG) && (p.set_value || show_description)) {
		exif_log (log, -1, "exif", _("You need to specify a tag!"));
		return 1;
	}
	if (((p.ifd < EXIF_IFD_0) || (p.ifd >= EXIF_IFD_COUNT)) &&
	    (p.set_value || show_description)) {
		exif_log (log, -1, "exif", _("You need to specify an IFD!"));
		return 1;
	}

	/* No command: Show help */
	if (argc <= 1) {
		poptPrintHelp (ctx, stdout, 0);
		poptFreeContext(ctx);
		return (1);
	}

	/* Commands not related to file. You can only specify one. */
	if (show_version) {
		printf ("%s\n", VERSION);
		exif_log_free (log);
		poptFreeContext (ctx);
		return 0;
	}
	if (show_description) {
		int rc = 0;
		const char *name = exif_tag_get_name_in_ifd (p.tag, p.ifd);
		if (!name) {
			exif_log (log, -1, "exif", _("Unknown Tag"));
			rc = 1;

		} else if (p.machine_readable) {
			/*
			 * The C() macro can point to a static buffer so these printfs
			 * must be done separately.
			 */
			printf ("0x%04x\t%s\t", p.tag,
				C(exif_tag_get_name_in_ifd (p.tag, p.ifd)));
			printf ("%s\t", C(exif_tag_get_title_in_ifd (p.tag, p.ifd)));
			printf ("%s\n",
				C(exif_tag_get_description_in_ifd (p.tag, p.ifd)));

		} else {
			printf (_("Tag '%s' "),
				C(exif_tag_get_title_in_ifd (p.tag, p.ifd)));
			printf (_("(0x%04x, '%s'): "), p.tag,
				C(exif_tag_get_name_in_ifd (p.tag, p.ifd)));
			printf ("%s\n",
				C(exif_tag_get_description_in_ifd (p.tag, p.ifd)));
		}

		exif_log_free (log);
		poptFreeContext (ctx);
		return rc;
	}

	/* Commands related to files */
	if (!((args = poptGetArgs (ctx)))) {
		if (!create_exif) {
			exif_log (log, -1, "exif", _("Specify input file or --create-exif"));
			poptPrintHelp (ctx, stdout, 0);
			exif_log_free (log);
			poptFreeContext (ctx);
			return 1;
		} else {
			/* Give a name to the synthesized EXIF tag set */
			static const char * const created_exif_name[] =
				{"(EXIF)", NULL};
			args = created_exif_name;
			continue_without_file = 1;
		}
	}

	while (*args) {
		ExifLoader *l;

		ed = NULL;
		p.fin = *args;
		if (!continue_without_file) {
			/* Identify the parameters */
			if (output)
				strncpy (fout, output, sizeof (fout) - 1);
			else {
				strncpy (fout, *args, sizeof (fout) - 1);
				strncat (fout, ".modified.jpeg",
					sizeof (fout) - strlen(fout) - 1);
				/* Should really abort if this file name is too long */
			}

			/*
			 * Try to read EXIF data from the file. 
			 * If there is no EXIF data, create it if the user 
			 * told us to do so.
			 */
			l = exif_loader_new ();
			exif_loader_log (l, log);
			if (create_exif)
				log_arg.ignore_corrupted = 1;
			exif_loader_write_file (l, *args);
			log_arg.ignore_corrupted = 0;
			if (!log_arg.corrupted)
				create_exif = 0;

			if (no_fixup)
				/* Override the default conversion options */
				ed = exif_get_data_opts(l, log, 0, EXIF_DATA_TYPE_UNKNOWN);
			else
				ed = exif_loader_get_data(l);

			exif_loader_unref (l);
		}
		if (!ed) {
			if (create_exif) {
				/* Create a new EXIF data set */
				ed = exif_data_new ();
				exif_data_log (ed, log);
				exif_data_set_data_type(ed, EXIF_DATA_TYPE_COMPRESSED);
				if (!no_fixup) {
					/* Add all the mandatory fields */
					exif_data_fix(ed);

					/* Create a new date tag */
					action_create_value (ed, log, EXIF_TAG_DATE_TIME, EXIF_IFD_0);
				}
			} else {
				exif_log (log, -1, "exif", _("'%s' is not "
				  "readable or does not contain EXIF data!"),
				  *args);
				return 1;
			}
		}

		/* These options can be used in conjunction with others */
		if (remove_thumb)
			action_remove_thumb (ed, log, p);
		if (p.set_thumb)
			action_insert_thumb (ed, log, p);

		/* These options are mutually exclusive */
		if (list_tags)
			action_tag_table (ed, p);
		else if ((p.tag != EXIF_INVALID_TAG) &&
			 !p.set_value && !remove_tag)
			action_show_tag (ed, log, p);
		else if (extract_thumbnail)
			action_save_thumb (ed, log, p, fout);
		else if (p.set_value)
			action_set_value (ed, log, p);
		else if (remove_tag)
			action_remove_tag (ed, log, p);
		else if (list_mnote) {
			if (xml_output) {
				exif_log (log, -1, "exif", _("XML format is "
				  "not available for Maker Notes"));
				return 1;
			}
			action_mnote_list (ed, p);
		} else if (p.machine_readable)
			action_tag_list_machine (ed, p);
		else if (xml_output)
			action_tag_list_xml (ed, p);
		else if (create_exif && !continue_without_file)
			/* Nothing here. Data will be saved later. */
			;
		else
			action_tag_list (ed, p);

		if (!continue_without_file && 
			(create_exif || p.set_thumb || remove_tag || remove_thumb ||
		     p.set_value))
			action_save (ed, log, p, fout);

		exif_data_unref (ed);

		args++;
	}

	/* Free all libpopt string arguments */
	free(tag_string);
	free(ifd_string);
	free(p.set_thumb);
	free(output);
	free(p.set_value);

	exif_log_free (log);
	poptFreeContext (ctx);

	return 0;
}
