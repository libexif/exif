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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-loader.h>

#include "libjpeg/jpeg-data.h"

#include "actions.h"
#include "exif-i18n.h"
#include "utils.h"

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

/* Old versions of popt.h don't define POPT_TABLEEND */
#ifndef POPT_TABLEEND
#  define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif

static void
internal_error (void)
{
	fprintf (stderr, _("Internal error. Please "
			   "contact <%s>.)"),
		 PACKAGE_BUGREPORT);
	fputc ('\n', stderr);
	exit (1);
}

static void
show_entry (ExifEntry *entry, unsigned int machine_readable)
{
	ExifIfd ifd = exif_entry_get_ifd (entry);

	if (machine_readable) {
		char b[1024];

		fprintf (stdout, "%s\n", C(exif_entry_get_value (entry, b, sizeof (b))));
		return;
	}

	printf (_("EXIF entry '%s' (0x%x, '%s') exists in IFD '%s':"),
		C(exif_tag_get_title_in_ifd (entry->tag, ifd)), entry->tag,
		C(exif_tag_get_name_in_ifd (entry->tag, ifd)), 
		C(exif_ifd_get_name (ifd)));
	printf ("\n");

	exif_entry_dump (entry, 0);
}

static void
search_entry (ExifData *ed, ExifTag tag, unsigned int machine_readable)
{
	ExifEntry *entry;
	unsigned int i;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		entry = exif_content_get_entry (ed->ifd[i], tag);
		if (entry)
			show_entry (entry, machine_readable);
	}
}

static void
convert_arg_to_entry (const char *set_value, ExifEntry *e, ExifByteOrder o)
{
	unsigned int i;
	char *value_p;

        /*
	 * ASCII strings are handled separately,
	 * since they don't require any conversion.
	 */
        if (e->format == EXIF_FORMAT_ASCII) {
		if (e->data) free (e->data);
		e->components = strlen (set_value) + 1;
		e->size = sizeof (char) * e->components;
		e->data = malloc (e->size);
                if (!e->data) {
                        fprintf (stderr, _("Not enough memory."));
                        fputc ('\n', stderr);
                        exit (1);
                }
                strcpy ((char *) e->data, (char *) set_value);
                return;
	}

        value_p = (char*) set_value;
	for (i = 0; i < e->components; i++) {
                const char *begin, *end;
                unsigned char *buf, s;
                const char comp_separ = ' ';

                begin = value_p;
		value_p = strchr (begin, comp_separ);
		if (!value_p) {
                        if (i != e->components - 1) {
                                fprintf (stderr, _("Too few components "
						   "specified!"));
				fputc ('\n', stderr);
				exit (1);
                        }
                        end = begin + strlen (begin);
                } else end = value_p++;

                buf = malloc ((end - begin + 1) * sizeof (char));
                strncpy ((char *) buf, (char *) begin, end - begin);
                buf[end - begin] = '\0';

		s = exif_format_get_size (e->format);
		switch (e->format) {
		case EXIF_FORMAT_ASCII:
			internal_error (); /* Previously handled */
			break;
		case EXIF_FORMAT_SHORT:
			exif_set_short (e->data + (s * i), o, atoi ((char *) buf));
			break;
		case EXIF_FORMAT_LONG:
			exif_set_long (e->data + (s * i), o, atol ((char *) buf));
			break;
		case EXIF_FORMAT_SLONG:
			exif_set_slong (e->data + (s * i), o, atol ((char *) buf));
			break;
		case EXIF_FORMAT_RATIONAL:
		case EXIF_FORMAT_SRATIONAL:
		case EXIF_FORMAT_BYTE:
		default:
			fprintf (stderr, _("Not yet implemented!"));
			fputc ('\n', stderr);
			exit (1);
		}
		free (buf);
	}
}

/* escape codes for output colors */
#define COL_BLUE   "\033[34m"
#define COL_GREEN  "\033[32m"
#define COL_RED    "\033[31m"
#define COL_NORMAL "\033[0m"

/* Ensure that we're actuall printing the escape codes to a TTY.
 * FIXME: We should make sure the terminal can actually understand these
 *        escape sequences
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

static void
log_func_exit (ExifLog *log, ExifLogCode code, const char *domain,
		const char *format, va_list args, void *data)
{
	switch (code) {
	case -1:
		put_colorstring (stderr, COL_RED);
		vfprintf (stderr, format, args);
		fprintf (stderr, "\n");
		put_colorstring (stderr, COL_NORMAL);
		exit (1);
	case EXIF_LOG_CODE_NO_MEMORY:
	case EXIF_LOG_CODE_CORRUPT_DATA:
		put_colorstring (stderr, COL_RED);
		fprintf (stderr, "%s (%s):\n", exif_log_code_get_title (code), domain);
		vfprintf (stderr, format, args);
		fprintf (stderr, "\n");
		put_colorstring (stderr, COL_NORMAL);
		exit (1);
	default:
		return;
	}
}

static int
save_exif_data_to_file (ExifData *ed, ExifLog *log, 
		const char *fname, const char *target)
{
	JPEGData *jdata;
	unsigned char *d = NULL;
	unsigned int ds;

	/* Parse the JPEG file. */
	jdata = jpeg_data_new ();
	jpeg_data_log (jdata, log);
	jpeg_data_load_file (jdata, fname);

	/* Make sure the EXIF data is not too big. */
	exif_data_save_data (ed, &d, &ds);
	if (ds) {
		if (ds > 0xffff) {
			fprintf (stderr, _("Too much EXIF data (%i bytes). "
				"Only %i bytes are allowed."), ds, 0xffff);
			fputc ('\n', stderr);
			return (1);
		}
		free (d);
	};

	jpeg_data_set_exif_data (jdata, ed);

	/* Save the modified image. */
	jpeg_data_save_file (jdata, target);
	jpeg_data_unref (jdata);

	fprintf (stdout, _("Wrote file '%s'."), target);
	fprintf (stdout, "\n");

	return (0);
}

static void
log_func (ExifLog *log, ExifLogCode code, const char *domain,
	  const char *format, va_list args, void *data)
{
	switch (code) {
	case -1:
		put_colorstring (stderr, COL_RED);
		vfprintf (stderr, format, args);
		fprintf (stderr, "\n");
		put_colorstring (stderr, COL_NORMAL);
		exit (1);
	case EXIF_LOG_CODE_DEBUG:
		put_colorstring (stdout, COL_GREEN);
		fprintf (stdout, "%s: ", domain);
		vfprintf (stdout, format, args);
		put_colorstring (stdout, COL_NORMAL);
		printf ("\n");
		break;
	case EXIF_LOG_CODE_NO_MEMORY:
	case EXIF_LOG_CODE_CORRUPT_DATA:
		put_colorstring (stderr, "\033[31;1m");
		put_colorstring (stderr, "\033[31;4m");
		fprintf (stderr, exif_log_code_get_title (code));
		fprintf (stderr, "\n");
		put_colorstring (stderr, "\033[;0m");
		put_colorstring (stderr, COL_RED);
		fprintf (stderr, exif_log_code_get_message (code));
		fprintf (stderr, "\n");
		fprintf (stderr, "%s: ", domain);
		vfprintf (stderr, format, args);
		put_colorstring (stderr, COL_NORMAL);
		fprintf (stderr, "\n");
		break;
	default:
		put_colorstring (stdout, COL_BLUE);
		printf ("%s: ", domain);
		vprintf (format, args);
		put_colorstring (stdout, COL_NORMAL);
		printf ("\n");
		break;
	}
}

typedef struct _ExifOptions ExifOptions;
struct _ExifOptions {
	unsigned int use_ids;
	ExifTag tag;
};

/*
 * Static variables. I had them first in main (), but people
 * compiling exif on IRIX complained about that not being compatible
 * with the "SGI MIPSpro C compiler". I don't understand and still think
 * these variables belong into main ().
 */
static unsigned int list_tags = 0, show_description = 0, machine_readable = 0;
static unsigned int xml_output = 0;
static unsigned int extract_thumbnail = 0, remove_thumbnail = 0;
static unsigned int remove_tag = 0;
static unsigned int list_mnote = 0, debug = 0;
static unsigned int show_version = 0;
static const char *set_value = NULL, *ifd_string = NULL, *tag_string = NULL;
static ExifIfd ifd = EXIF_IFD_0;
static ExifTag tag = 0;
static ExifOptions eo = {0, 0};

int
main (int argc, const char **argv)
{
	/* POPT_ARG_NONE needs an int, not char! */
	poptContext ctx;
	const char **args, *output = NULL;
	const char *ithumbnail = NULL;
	struct poptOption options[] = {
		POPT_AUTOHELP
    {"version", 'v', POPT_ARG_NONE, &show_version, 0,
      N_("Display software version"), NULL},
		{"ids", 'i', POPT_ARG_NONE, &eo.use_ids, 0,
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
		{"remove-thumbnail", 'r', POPT_ARG_NONE, &remove_thumbnail, 0,
		 N_("Remove thumbnail"), NULL},
		{"insert-thumbnail", 'n', POPT_ARG_STRING, &ithumbnail, 0,
		 N_("Insert FILE as thumbnail"), N_("FILE")},
		{"output", 'o', POPT_ARG_STRING, &output, 0,
		 N_("Write data to FILE"), N_("FILE")},
		{"set-value", '\0', POPT_ARG_STRING, &set_value, 0,
		 N_("Value"), NULL},
		{"machine-readable", 'm', POPT_ARG_NONE, &machine_readable, 0,
		 N_("Output in a machine-readable (tab delimited) format"),
		 NULL},
		{"xml-output", 'x', POPT_ARG_NONE, &xml_output, 0,
		 N_("Output in a XML format"),
		 NULL},
		{"debug", 'd', POPT_ARG_NONE, &debug, 0,
		 N_("Show debugging messages"), NULL},
		POPT_TABLEEND};
	ExifData *ed;
	ExifEntry *e;
	char fname[1024];
	FILE *f;
	ExifLog *log = NULL;

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
	setlocale (LC_ALL, "");
#endif
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);
	poptSetOtherOptionHelp (ctx, _("[OPTION...] file"));
	while (poptGetNextOpt (ctx) > 0);

	/*
	 * When debugging, continue as far as possible. If not, make all errors
	 * fatal.
	 */
	log = exif_log_new ();
	exif_log_set_func (log, debug ? log_func : log_func_exit, NULL);

	/* Any command line parameters ? */
	if (argc <= 1) {
		poptPrintHelp (ctx, stdout, 0);
		return (1);
	}

  if (show_version) {
    printf ("%s\n", VERSION);
    return 0;
  }

	if (ifd_string) {
		ifd = exif_ifd_from_string (ifd_string);
		if ((ifd < EXIF_IFD_0) || (ifd >= EXIF_IFD_COUNT) ||
		    !exif_ifd_get_name (ifd))
			exif_log (log, -1, "exif", _("Invalid IFD '%s'. Valid IFDs are "
						"'0', '1', 'EXIF', 'GPS', and "
						"'Interoperability'."), ifd_string);
	}

	if (tag_string) {
		tag = exif_tag_from_string (tag_string);
		if (tag == 0xffff)
			exif_log (log, -1, "exif", _("Invalid tag '%s'!"), tag_string);
		eo.tag = tag;
	}

	if (show_description) {
		if (!eo.tag) exif_log (log, -1, "exif", _("Please specify a tag!"));
		printf (_("Tag '%s' (0x%04x, '%s'): %s"),
			C(exif_tag_get_title_in_ifd (eo.tag, ifd)), eo.tag,
			C(exif_tag_get_name_in_ifd (eo.tag, ifd)),
			C(exif_tag_get_description_in_ifd (eo.tag, ifd)));
		printf ("\n");
		return (0);
	}

	args = poptGetArgs (ctx);

	if (args) {
		while (*args) {
			ExifLoader *l;

			/*
			 * Try to read EXIF data from the file. 
			 * If there is no EXIF data, exit.
			 */
			l = exif_loader_new ();
			exif_loader_log (l, log);
			exif_loader_write_file (l, *args);
			ed = exif_loader_get_data (l);
			exif_loader_unref (l);
			if (!ed || ! (ed->data || ed->size ||
						ed->ifd[EXIF_IFD_0]->count ||
						ed->ifd[EXIF_IFD_1]->count ||
						ed->ifd[EXIF_IFD_EXIF]->count ||
						ed->ifd[EXIF_IFD_GPS]->count ||
						ed->ifd[EXIF_IFD_INTEROPERABILITY]->count))
				exif_log (log, -1, "exif", _("'%s' does not "
							"contain EXIF data!"), *args);

			/* Where do we save the output? */
			memset (fname, 0, sizeof (fname));
			if (output)
				strncpy (fname, output, sizeof (fname) - 1);
			else {
				strncpy (fname, *args, sizeof (fname) - 1);
				strncat (fname, ".modified.jpeg",
					 sizeof (fname) - 1);
			}

			if (list_tags) {
				action_tag_table (*args, ed);
			} else if (tag && !set_value && !remove_tag) {
				if ((ifd >= EXIF_IFD_0) &&
				    (ifd < EXIF_IFD_COUNT)) {
					e = exif_content_get_entry (
							ed->ifd[ifd], tag);
					if (e)
						show_entry (e, machine_readable);
					else
						exif_log (log, -1, "exif", _("IFD '%s' "
									"does not contain tag '%s'."), ifd_string, tag_string);
				} else {
					search_entry (ed, eo.tag, machine_readable);
				}
			} else if (extract_thumbnail) {

				/* No thumbnail? Exit. */
				if (!ed->data)
					exif_log (log, -1, "exif", _("'%s' does not "
								"contain a thumbnail!"), *args);

				/* Save the thumbnail */
				f = fopen (fname, "wb");
				if (!f)
#ifdef __GNUC__
					exif_log (log, -1, "exif",
						_("Could not open '%s' for "
						"writing (%m)!"), fname);
#else
					exif_log (log, -1, "exif", 
						_("Could not open '%s' for "
						"writing (%s)!"), fname,
						strerror (errno));
#endif
				fwrite (ed->data, 1, ed->size, f);
				fclose (f);
				fprintf (stdout, _("Wrote file '%s'."),
					 fname);
				fprintf (stdout, "\n");

			} else if (remove_thumbnail) {

				/* Get rid of the thumbnail */
				if (ed->data) {
					free (ed->data);
					ed->data = NULL;
				}
				ed->size = 0;

				/* Save the new data. */
				save_exif_data_to_file (ed, log, *args, fname);

			} else if (ithumbnail) {

				/* Get rid of the old thumbnail */
				if (ed->data) {
					free (ed->data);
					ed->data = NULL;
				}
				ed->size = 0;

				/* Insert new thumbnail */
				f = fopen (ithumbnail, "rb");
				if (!f)
#ifdef __GNUC__
					exif_log (log, -1, "exif", _("Could not open "
						"'%s' (%m)!"), ithumbnail);
#else
					exif_log (log, -1, "exif", _("Could not open "
						"'%s' (%s)!"), ithumbnail,
						strerror (errno));
#endif
				fseek (f, 0, SEEK_END);
				ed->size = ftell (f);
				ed->data = malloc (sizeof (char) * ed->size);
				if (ed->size && !ed->data) EXIF_LOG_NO_MEMORY (log, "exif", ed->size);
				fseek (f, 0, SEEK_SET);
				if (fread (ed->data, sizeof (char),
					   ed->size, f) != ed->size)
#ifdef __GNUC__
					exif_log (log, -1, "exif", _("Could not read "
						"'%s' (%m)."), ithumbnail);
#else
					exif_log (log, -1, "exif", _("Could not read "
						"'%s' (%s)."), ithumbnail,
						strerror (errno));
#endif
				fclose (f);

				save_exif_data_to_file (ed, log, *args, fname);

			} else if (set_value) {

				/* We need a tag... */
				if (!tag) exif_log (log, -1, "exif", _("You need to specify a tag!"));

				/* ... and an IFD. */
				if ((ifd < EXIF_IFD_0) ||
				    (ifd >= EXIF_IFD_COUNT))
					exif_log (log, -1, "exif", _("You need to specify an IFD!"));

				/* If the entry doesn't exist, create it. */
				e = exif_content_get_entry (ed->ifd[ifd], tag);
				if (!e) {
				    e = exif_entry_new ();
				    exif_content_add_entry (ed->ifd[ifd], e);
				    exif_entry_initialize (e, tag);
				}

				/* Now set the value and save the data. */
                                convert_arg_to_entry (set_value, e,
						exif_data_get_byte_order (ed));
				save_exif_data_to_file (ed, log, *args, fname);
			} else if (remove_tag) {
				
				/* We need an IFD. */
				if ((ifd < EXIF_IFD_0) ||
				    (ifd >= EXIF_IFD_COUNT))
					exif_log (log, -1, "exif", _("You need to specify an IFD!"));

				if (!tag) {
					while (ed->ifd[ifd] &&
					       ed->ifd[ifd]->count)
						exif_content_remove_entry (
						    ed->ifd[ifd],
						    ed->ifd[ifd]->entries[0]);
				} else {
					e = exif_content_get_entry (
							ed->ifd[ifd], tag);
					if (!e)
					    exif_log (log, -1, "exif", _("IFD '%s' does not contain a "
						 "tag '%s'!"),
						exif_ifd_get_name (ifd),
						exif_tag_get_name_in_ifd (tag, ifd));
					exif_content_remove_entry (ed->ifd[ifd], e);
				}

				/* Save modified data. */
				save_exif_data_to_file (ed, log, *args, fname);

			} else if (machine_readable) {
				action_tag_list_machine (*args, ed, eo.use_ids);
			} else if (xml_output) {
				action_tag_list_xml (*args, ed, eo.use_ids);
			} else if (list_mnote) {
				action_mnote_list (*args, ed, eo.use_ids);
			} else
				action_tag_list (*args, ed, eo.use_ids);
			exif_data_unref (ed);
			args++;
		}
	} else {
		poptPrintHelp (ctx, stdout, 0);
		return(1);
	}

	poptFreeContext (ctx);

	return (0);
}
