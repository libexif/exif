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

/* Must be loaded after exif-i18n.h */
#include <popt.h>

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

#ifdef ENABLE_GLIBC_MEMDEBUG
#include <mcheck.h>
#endif

/* Old versions of popt.h don't define POPT_TABLEEND */
#ifndef POPT_TABLEEND
#  define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif

static void
internal_error (void)
{
	fprintf (stderr, _("Internal error. Please "
			   "contact <%s>."),
		 PACKAGE_BUGREPORT);
	fputc ('\n', stderr);
	exit (1);
}

static void
convert_arg_to_entry (const char *set_value, ExifEntry *e, ExifByteOrder o)
{
	unsigned int i, numcomponents;
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

	/*
	 * Make sure we can handle this entry
	 */
	if ((e->components == 0) && *set_value) {
		fprintf (stderr, _("Setting a value for this tag "
				   "is unsupported!"));
		fputc ('\n', stderr);
		exit (1);
	}

        value_p = (char*) set_value;
	numcomponents = e->components;
	for (i = 0; i < numcomponents; ++i) {
                const char *begin, *end;
                unsigned char *buf, s;
		static const char comp_separ = ' ';

                begin = value_p;
		value_p = strchr (begin, comp_separ);
		if (!value_p) {
                        if (i != numcomponents - 1) {
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
		case EXIF_FORMAT_SSHORT:
			exif_set_sshort (e->data + (s * i), o, atoi ((char *) buf));
			break;
		case EXIF_FORMAT_RATIONAL:
			/*
			 * Hack to simplify the loop for rational numbers.
			 * Should really be using exif_set_rational instead
			 */
			if (i == 0) numcomponents *= 2;
			s /= 2;
			/* Fall through to LONG handler */
		case EXIF_FORMAT_LONG:
			exif_set_long (e->data + (s * i), o, atol ((char *) buf));
			break;
		case EXIF_FORMAT_SRATIONAL:
			/*
			 * Hack to simplify the loop for rational numbers.
			 * Should really be using exif_set_srational instead
			 */
			if (i == 0) numcomponents *= 2;
			s /= 2;
			/* Fall through to SLONG handler */
		case EXIF_FORMAT_SLONG:
			exif_set_slong (e->data + (s * i), o, atol ((char *) buf));
			break;
		case EXIF_FORMAT_BYTE:
		case EXIF_FORMAT_SBYTE:
		case EXIF_FORMAT_FLOAT:
		case EXIF_FORMAT_DOUBLE:
		case EXIF_FORMAT_UNDEFINED:
		default:
			fprintf (stderr, _("Not yet implemented!"));
			fputc ('\n', stderr);
			exit (1);
		}
		free (buf);
	}
	if (value_p && *value_p) {
		fprintf (stderr, _("Warning; Too many components specified!"));
		fputc ('\n', stderr);
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
save_exif_data_to_file (ExifData *ed, ExifLog *log, ExifParams p)
{
	JPEGData *jdata;
	unsigned char *d = NULL;
	unsigned int ds;

	/* Parse the JPEG file. */
	jdata = jpeg_data_new ();
	jpeg_data_log (jdata, log);
	jpeg_data_load_file (jdata, p.fin);

	/* Make sure the EXIF data is not too big. */
	exif_data_save_data (ed, &d, &ds);
	if (ds) {
		free (d);
		if (ds > 0xffff) {
			fprintf (stderr, _("Too much EXIF data (%i bytes). "
				"Only %i bytes are allowed."), ds, 0xffff);
			fputc ('\n', stderr);
			return (1);
		}
	};

	jpeg_data_set_exif_data (jdata, ed);

	/* Save the modified image. */
	jpeg_data_save_file (jdata, p.fout);
	jpeg_data_unref (jdata);

	fprintf (stdout, _("Wrote file '%s'."), p.fout);
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

/*
 * Static variables. I had them first in main (), but people
 * compiling exif on IRIX complained about that not being compatible
 * with the "SGI MIPSpro C compiler". I don't understand and still think
 * these variables belong into main ().
 */
static unsigned int list_tags = 0, show_description = 0;
static unsigned int xml_output = 0;
static unsigned int extract_thumbnail = 0, remove_thumbnail = 0;
static unsigned int remove_tag = 0;
static unsigned int list_mnote = 0, debug = 0;
static unsigned int show_version = 0;
static const char *set_value = NULL, *ifd_string = NULL, *tag_string = NULL;
static ExifParams p = {0, EXIF_IFD_COUNT, 0, 0, NULL, {0, }};

int
main (int argc, const char **argv)
{
	/* POPT_ARG_NONE needs an int, not char! */
	poptContext ctx;
	const char **args, *output = NULL;
	const char *ithumbnail = NULL;
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
		{"remove-thumbnail", 'r', POPT_ARG_NONE, &remove_thumbnail, 0,
		 N_("Remove thumbnail"), NULL},
		{"insert-thumbnail", 'n', POPT_ARG_STRING, &ithumbnail, 0,
		 N_("Insert FILE as thumbnail"), N_("FILE")},
		{"output", 'o', POPT_ARG_STRING, &output, 0,
		 N_("Write data to FILE"), N_("FILE")},
		{"set-value", '\0', POPT_ARG_STRING, &set_value, 0,
		 N_("Value"), NULL},
		{"machine-readable", 'm', POPT_ARG_NONE, &p.machine_readable, 0,
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
	FILE *f;
	ExifLog *log = NULL;

#ifdef ENABLE_GLIBC_MEMDEBUG
	mcheck(NULL);
	mtrace();
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

	/* Any command line parameters ? */
	if (argc <= 1) {
		poptPrintHelp (ctx, stdout, 0);
		poptFreeContext(ctx);
		return (1);
	}

	/*
	 * When debugging, continue as far as possible. If not, make all errors
	 * fatal.
	 */
	log = exif_log_new ();
	exif_log_set_func (log, debug ? log_func : log_func_exit, NULL);

	if (show_version) {
		printf ("%s\n", VERSION);
		exif_log_free(log);
		poptFreeContext(ctx);
		return 0;
	}

	/* Identify the parameters */
	if (ifd_string) {
		p.ifd = exif_ifd_from_string (ifd_string);
		if ((p.ifd < EXIF_IFD_0) || (p.ifd >= EXIF_IFD_COUNT) ||
		    !exif_ifd_get_name (p.ifd))
			exif_log (log, -1, "exif",
				_("Invalid IFD '%s'. Valid IFDs are "
				"'0', '1', 'EXIF', 'GPS', and "
				"'Interoperability'."), ifd_string);
	}
	if (tag_string) {
		p.tag = exif_tag_from_string (tag_string);
		if (p.tag == 0xffff) {
			exif_log (log, -1, "exif", _("Invalid tag '%s'!"),
				tag_string);
			p.tag = 0;
		}
	}
	memset (p.fout, 0, sizeof (p.fout));
	if (output)
		strncpy (p.fout, output, sizeof (p.fout) - 1);

	/* Check for all necessary parameters */
	if (!p.tag && (set_value || show_description))
		exif_log (log, -1, "exif", _("You need to specify a tag!")); 
	if (((p.ifd < EXIF_IFD_0) || (p.ifd >= EXIF_IFD_COUNT)) &&
	    (set_value || remove_tag))
		exif_log (log, -1, "exif", _("You need to specify an IFD!"));

	if (show_description) {
		/*
		 * The C() macro can point to a static buffer so these printfs
		 * must be done separately.
		 */
		printf (_("Tag '%s' "),
			C(exif_tag_get_title_in_ifd (p.tag, p.ifd)));
		printf (_("(0x%04x, '%s'): "), p.tag,
			C(exif_tag_get_name_in_ifd (p.tag, p.ifd)));
		printf ("%s\n",
			C(exif_tag_get_description_in_ifd (p.tag, p.ifd)));

		exif_log_free(log);
		poptFreeContext(ctx);
		return (0);
	}

	args = poptGetArgs (ctx);

	if (args) {
		while (*args) {
			ExifLoader *l;

			/* Identify the parameters */
			if (!output) {
				strncpy (p.fout, *args, sizeof (p.fout) - 1);
				strncat (p.fout, ".modified.jpeg",
					 sizeof (p.fout) - 1);
			}
			p.fin = *args;

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
						ed->ifd[EXIF_IFD_INTEROPERABILITY]->count)) {
				exif_log (log, -1, "exif", _("'%s' does not "
							"contain EXIF data!"), *args);
				/* Never gets here--exif_log has exit()ed */
			}

			if (list_tags) {
				action_tag_table (ed, p);

			} else if (p.tag && !set_value && !remove_tag) {
				action_show_tag (ed, log, p);

			} else if (extract_thumbnail) {
				action_extract_thumb (ed, log, p);

			} else if (remove_thumbnail) {

				/* Get rid of the thumbnail */
				if (ed->data) {
					free (ed->data);
					ed->data = NULL;
				}
				ed->size = 0;

				/* Save the new data. */
				save_exif_data_to_file (ed, log, p);

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

				save_exif_data_to_file (ed, log, p);

			} else if (set_value) {

				/* If the entry doesn't exist, create it. */
				e = exif_content_get_entry (ed->ifd[p.ifd], p.tag);
				if (!e) {
				    e = exif_entry_new ();
				    exif_content_add_entry (ed->ifd[p.ifd], e);
				    exif_entry_initialize (e, p.tag);
				}

				/* Now set the value and save the data. */
                                convert_arg_to_entry (set_value, e,
						exif_data_get_byte_order (ed));
				save_exif_data_to_file (ed, log, p);

			} else if (remove_tag) {

				if (!p.tag) {
					while (ed->ifd[p.ifd] &&
					       ed->ifd[p.ifd]->count)
						exif_content_remove_entry (
						    ed->ifd[p.ifd],
						    ed->ifd[p.ifd]->entries[0]);
				} else {
					e = exif_content_get_entry (
							ed->ifd[p.ifd], p.tag);
					if (!e)
					    exif_log (log, -1, "exif", _("IFD '%s' does not contain a "
						 "tag '%s'!"),
						exif_ifd_get_name (p.ifd),
						exif_tag_get_name_in_ifd (p.tag, p.ifd));
					exif_content_remove_entry (ed->ifd[p.ifd], e);
				}

				/* Save modified data. */
				save_exif_data_to_file (ed, log, p);

			} else if (p.machine_readable) {
				action_tag_list_machine (ed, p);

			} else if (xml_output) {
				action_tag_list_xml (ed, p);

			} else if (list_mnote) {
				action_mnote_list (ed, p);

			} else
				action_tag_list (ed, p);

			exif_data_unref (ed);
			args++;
		}
	} else {
		poptPrintHelp (ctx, stdout, 0);
		exif_log_free(log);
		poptFreeContext(ctx);
		return(1);
	}

	exif_log_free(log);
	poptFreeContext (ctx);

	return (0);
}
