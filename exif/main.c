/* main.c
 *
 * Copyright (C) 2002 Lutz Müller <lutz@users.sourceforge.net>
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

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>

#include <libexif/exif-data.h>
#include <libexif/exif-note.h>
#include <libexif/exif-utils.h>

#include "libjpeg/jpeg-data.h"

#include "actions.h"
#include "utils.h"

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#ifdef HAVE_LOCAL_H
#  include <locale.h>
#endif

/* Old versions of popt.h don't define POPT_TABLEEND */
#ifndef POPT_TABLEEND
#  define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif

static void
show_maker_note (ExifEntry *entry)
{
	ExifNote *note;
	char **value;
	unsigned int i;

	note = exif_note_new_from_data (entry->data, entry->size);
	if (!note) {
		fprintf (stderr, _("Could not parse data of tag '%s'."), 
			exif_tag_get_name (entry->tag));
		fputc ('\n', stderr);
		return;
	}

	value = exif_note_get_value (note);
	exif_note_unref (note);

	if (!value || !value[0]) {
		fprintf (stderr, 
			_("Tag '%s' does not contain known information."),
			exif_tag_get_name (entry->tag));
		fputc ('\n', stderr);
		return;
	} else if (!value[1]) {
		printf (_("Tag '%s' contains one piece of information:"),
			exif_tag_get_name (entry->tag));
		printf ("\n");
	} else {
		printf (_("Tag '%s' contains the following information:"),
			exif_tag_get_name (entry->tag));
		printf ("\n");
	}
	for (i = 0; value && value[i]; i++) {
		printf (" %3i %s\n", i, value[i]);
		free (value[i]);
	}
	free (value);
}

static void
show_entry (ExifEntry *entry, const char *caption)
{
	unsigned int i;

	printf (_("EXIF entry '%s' (0x%x, '%s') exists in IFD '%s':"),
		exif_tag_get_title (entry->tag), entry->tag,
		exif_tag_get_name (entry->tag), caption);
	printf ("\n");
	printf (_("  Format: '%s'"), exif_format_get_name (entry->format));
	printf ("\n");
	printf (_("  Components: %i"), (int) entry->components);
	printf ("\n");
	printf (_("  Value: '%s'"), exif_entry_get_value (entry));
	printf ("\n");
	printf (_("  Data:"));
	for (i = 0; i < entry->size; i++) {
		if (!(i % 10))
			printf ("\n    ");
		printf ("0x%02x ", entry->data[i]);
	}
	printf ("\n");
	if (entry->tag == EXIF_TAG_MAKER_NOTE)
		show_maker_note (entry);
}

static void
search_entry (ExifData *ed, ExifTag tag)
{
	ExifEntry *entry;
	unsigned int i;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		entry = exif_content_get_entry (ed->ifd[i], tag);
		if (entry)
			show_entry (entry, exif_ifd_get_name (i));
	}
}

static int
save_exif_data_to_file (ExifData *ed, const char *fname, const char *target)
{
	JPEGData *jdata;
	unsigned char *d = NULL;
	unsigned int ds;

	/* Parse the JPEG file */
	jdata = jpeg_data_new_from_file (fname);
	if (!jdata) {
		fprintf (stderr, _("Could not parse JPEG file '%s'."),
			 fname);
		fputc ('\n', stderr);
		return (1);
	}

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

typedef struct _ExifOptions ExifOptions;
struct _ExifOptions {
	unsigned char use_ids;
	ExifTag tag;
};

int
main (int argc, const char **argv)
{
	/* POPT_ARG_NONE needs an int, not char! */
	unsigned int list_tags = 0, show_description = 0;
	unsigned int extract_thumbnail = 0, remove_thumbnail = 0;
	unsigned int remove = 0;
	const char *set_value = NULL, *ifd_string = NULL, *tag_string = NULL;
	ExifIfd ifd = -1;
	ExifTag tag = 0;
	ExifOptions eo = {0, 0};
	poptContext ctx;
	const char **args, *output = NULL;
	const char *ithumbnail = NULL;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"ids", 'i', POPT_ARG_NONE, &eo.use_ids, 0,
		 N_("Show IDs instead of tag names"), NULL},
		{"tag", 't', POPT_ARG_STRING, &tag_string, 0,
		 N_("Select tag"), N_("tag")},
		{"ifd", '\0', POPT_ARG_STRING, &ifd_string, 0,
		 N_("Select IFD"), N_("IFD")},
		{"list-tags", 'l', POPT_ARG_NONE, &list_tags, 0,
		 N_("List all EXIF tags"), NULL},
		{"remove", '\0', POPT_ARG_NONE, &remove, 0,
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
		 N_("Write output to FILE"), N_("FILE")},
		{"set-value", '\0', POPT_ARG_STRING, &set_value, 0,
		 N_("Value"), NULL},
		POPT_TABLEEND};
	ExifData *ed;
	ExifEntry *e;
	char fname[1024];
	FILE *f;

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
	setlocale (LC_ALL, "");
#endif
	bindtextdomain (PACKAGE, EXIF_LOCALEDIR);
	textdomain (PACKAGE);
#endif

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);
	poptSetOtherOptionHelp (ctx, _("[OPTION...] file"));
	while (poptGetNextOpt (ctx) > 0);

	/* Any option? */
	if (argc <= 1) {
		poptPrintUsage (ctx, stdout, 0);
		return (0);
	}

	if (tag_string) {
		tag = exif_tag_from_string (tag_string);
		if (!tag || !exif_tag_get_name (tag)) {
			fprintf (stderr, _("Invalid tag '%s'!"), tag_string);
			fputc ('\n', stderr);
			return (1);
		}
		eo.tag = tag;
	}

	if (ifd_string) {
		ifd = exif_ifd_from_string (ifd_string);
		if ((ifd < 0) || !exif_ifd_get_name (ifd)) {
			fprintf (stderr, _("Invalid IFD '%s'. Valid IFDs are "
				"'0', '1', 'EXIF', 'GPS', and "
				"'Interoperability'."), ifd_string);
			fputc ('\n', stderr);
			return (1);
		}
	}

	if (show_description) {
		if (!eo.tag) {
			fprintf (stderr, _("Please specify a tag!"));
			fputc ('\n', stderr);
			return (1);
		}
		printf (_("Tag '%s' (0x%04x, '%s'): %s"),
			exif_tag_get_title (eo.tag), eo.tag,
			exif_tag_get_name (eo.tag),
			exif_tag_get_description (eo.tag));
		printf ("\n");
		return (0);
	}

	args = poptGetArgs (ctx);

	if (args) {
		while (*args) {

			/*
			 * Try to read EXIF data from the file. 
			 * If there is no EXIF data, exit.
			 */
			ed = exif_data_new_from_file (*args);
			if (!ed) {
				fprintf (stderr, _("'%s' does not "
					 "contain EXIF data!"), *args);
				fputc ('\n', stderr);
				exit (1);
			}

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
			} else if (tag && !set_value) {
				if (ifd >= 0) {
					e = exif_content_get_entry (
							ed->ifd[ifd], tag);
					if (e)
						show_entry (e, ifd_string);
					else {
						fprintf (stderr, _("IFD '%s' "
							"does not contain tag "
							"'%s'."), 
							ifd_string, tag_string);
						fputc ('\n', stderr);
						return (1);
					}
				} else {
					search_entry (ed, eo.tag);
				}
			} else if (extract_thumbnail) {

				/* No thumbnail? Exit. */
				if (!ed->data) {
					fprintf (stderr, _("'%s' does not "
						"contain a thumbnail!"),
						*args);
					fputc ('\n', stderr);
					return (1);
				}

				/* Save the thumbnail */
				f = fopen (fname, "wb");
				if (!f) {
#ifdef __GNUC__
					fprintf (stderr,
						_("Could not open '%s' for "
						"writing (%m)!"), fname);
#else
					fprintf (stderr,
						_("Could not open '%s' for "
						"writing (%s)!"), fname,
						strerror (errno));
#endif
					fputc ('\n', stderr);
					return (1);
				}
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

			} else if (ithumbnail) {

				/* Get rid of the old thumbnail */
				if (ed->data) {
					free (ed->data);
					ed->data = NULL;
				}
				ed->size = 0;

				/* Insert new thumbnail */
				f = fopen (ithumbnail, "rb");
				if (!f) {
#ifdef __GNUC__
					fprintf (stderr, _("Could not open "
						"'%s' (%m)!"), ithumbnail);
#else
					fprintf (stderr, _("Could not open "
						"'%s' (%s)!"), ithumbnail,
						strerror (errno));
#endif
					fputc ('\n', stderr);
					return (1);
				}
				fseek (f, 0, SEEK_END);
				ed->size = ftell (f);
				ed->data = malloc (sizeof (char) * ed->size);
				if (ed->size && !ed->data) {
					fprintf (stderr, _("Could not "
						"allocate %i byte(s)."),
						ed->size);
					fputc ('\n', stderr);
					return (1);
				}
				fseek (f, 0, SEEK_SET);
				if (fread (ed->data, sizeof (char),
					   ed->size, f) != ed->size) {
#ifdef __GNUC__
					fprintf (stderr, _("Could not read "
						"'%s' (%m)."), ithumbnail);
#else
					fprintf (stderr, _("Could not read "
						"'%s' (%s)."), ithumbnail,
						strerror (errno));
#endif
					fputc ('\n', stderr);
					return (1);
				}
				fclose (f);

				save_exif_data_to_file (ed, *args, fname);

			} else if (set_value) {

				/* We need a tag... */
				if (!tag) {
					fprintf (stderr, _("You need to "
						"specify a tag!"));
					fputc ('\n', stderr);
					return (1);
				}

				/* ... and an IFD. */
				if (ifd < 0) {
					fprintf (stderr, _("You need to "
						"specify an IFD!"));
					fputc ('\n', stderr);
					return (1);
				}

				e = exif_content_get_entry (ed->ifd[ifd], tag);
				if (!e) {
				    e = exif_entry_new ();
				    exif_content_add_entry (ed->ifd[ifd], e);
				    exif_entry_initialize (e, tag);
				}
{
	unsigned int begin = 0, end = 0, i;
	unsigned char buf[1024], s;
	ExifByteOrder o;

	o = exif_data_get_byte_order (ed);
	for (i = 0; i < e->components; i++) {
		memset (buf, 0, sizeof (buf));
		for (begin = end; end < strlen (set_value); end++) {
			if (set_value[end] == '\\') {
				end++;
				if (set_value[end] == '\\') {
					buf[strlen(buf)] = set_value[end];
				} else if (set_value[end] == ' ') {
					buf[strlen(buf)] = set_value[end];
				} else {
					fprintf (stderr, "Wrong masking! "
						"'\\\\' will be interpreted "
						"as '\\', '\\ ' as ' '. "
						"'\\' followed by anything "
						"except '\\' or ' ' is "
						"invalid.");
					fputc ('\n', stderr);
					exit (1);
				}
			} else if (set_value[end] == ' ') {
				break;
			} else
				buf[strlen(buf)] = set_value[end];
		}
		s = exif_format_get_size (e->format);
		switch (e->format) {
		case EXIF_FORMAT_BYTE:
			/* e->data[s * i] = ; */

			fprintf (stderr, _("Not yet implemented!"));
			fputc ('\n', stderr);
			return (1);

			break;
		case EXIF_FORMAT_ASCII:
			if (i != 0) {
				fprintf (stderr, _("Internal error. Please "
					"contact <libexif-devel@"
					"lists.sourceforge.net>."));
				fputc ('\n', stderr);
				return (1);
			}
			if (e->data)
				free (e->data);
			e->size = strlen (buf) + 1;
			e->data = malloc (sizeof (char) * e->size);
			if (!e->data) {
				fprintf (stderr, _("Not enough memory."));
				fputc ('\n', stderr);
				return (1);
			}
			strcpy (e->data, buf);
			break;
		case EXIF_FORMAT_SHORT:
			exif_set_short (e->data + (s * i), o, atoi (buf));
			break;
		case EXIF_FORMAT_LONG:
			exif_set_long (e->data + (s * i), o, atol (buf));
			break;
		case EXIF_FORMAT_SLONG:
			exif_set_slong (e->data + (s * i), o, atol (buf));
			break;
		case EXIF_FORMAT_RATIONAL:
		case EXIF_FORMAT_SRATIONAL:
		default:
			fprintf (stderr, _("Not yet implemented!"));
			fputc ('\n', stderr);
			return (1);
		}
	}
}
				save_exif_data_to_file (ed, *args, fname);
			} else if (remove) {
				
				/* We need an IFD. */
				if (ifd < 0) {
					fprintf (stderr, _("You need to "
						 "specify an IFD!"));
					fputc ('\n', stderr);
					return (1);
				}

				if (!tag) {
					while (ed->ifd[ifd]->count)
						exif_content_remove_entry (
						    ed->ifd[ifd],
						    ed->ifd[ifd]->entries[0]);
				} else {
					e = exif_content_get_entry (
							ed->ifd[ifd], tag);
					if (!e) {
					    fprintf (stderr, _("IFD '%s' "
						 "does not contain a "
						 "tag '%s'!"),
						exif_ifd_get_name (ifd),
						exif_tag_get_name (tag));
					    fputc ('\n', stderr);
					    return (1);
					}
					exif_content_remove_entry (ed->ifd[ifd],
								   e);
				}
			} else
				action_tag_list (*args, ed, eo.use_ids);
			exif_data_unref (ed);
			args++;
		}
	}

	poptFreeContext (ctx);

	return (0);
}
