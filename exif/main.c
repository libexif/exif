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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>

#include <libexif/exif-data.h>
#include <libexif/exif-note.h>

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
		fprintf (stderr, "\n");
		return;
	}

	value = exif_note_get_value (note);
	exif_note_unref (note);

	if (!value || !value[0]) {
		fprintf (stderr, 
			_("Tag '%s' does not contain known information."),
			exif_tag_get_name (entry->tag));
		fprintf (stderr, "\n");
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

	printf (_("EXIF entry '%s' (0x%x, '%s') exists in '%s':"),
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

	entry = exif_content_get_entry (ed->ifd0, tag);
	if (entry)
		show_entry (entry, _("IFD 0"));

	entry = exif_content_get_entry (ed->ifd_exif, tag);
	if (entry)
		show_entry (entry, _("EXIF IFD"));

	entry = exif_content_get_entry (ed->ifd_gps, tag);
	if (entry)
		show_entry (entry, _("GPS IFD"));

	entry = exif_content_get_entry (ed->ifd1, tag);
	if (entry)
		show_entry (entry, _("IFD 1"));

	entry = exif_content_get_entry (ed->ifd_interoperability, tag);
	if (entry)
		show_entry (entry, _("Interoperability IFD"));
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
	unsigned int extract_thumbnail = 0;

	ExifOptions eo = {0, 0};
	poptContext ctx;
	const char **args, *tag = NULL, *output = NULL;
	const char *ithumbnail = NULL;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"ids", 'i', POPT_ARG_NONE, &eo.use_ids, 0,
		 N_("Show IDs instead of tag names"), NULL},
		{"tag", 't', POPT_ARG_STRING, &tag, 0,
		 N_("Select tag"), N_("tag")},
		{"list-tags", 'l', POPT_ARG_NONE, &list_tags, 0,
		 N_("List all EXIF tags"), NULL},
		{"show-description", 's', POPT_ARG_NONE, &show_description, 0,
		 N_("Show description of tag"), NULL},
		{"extract-thumbnail", 'e', POPT_ARG_NONE, &extract_thumbnail, 0,
		 N_("Extract thumbnail"), NULL},
		{"insert-thumbnail", 'n', POPT_ARG_STRING, &ithumbnail, 0,
		 N_("Insert FILE as thumbnail"), N_("FILE")},
		{"output", 'o', POPT_ARG_STRING, &output, 0,
		 N_("Write output to FILE"), N_("FILE")},
		POPT_TABLEEND};
	ExifData *ed;
	char filename[1024];
	FILE *f;
	JPEGData *jdata;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, EXIF_LOCALEDIR);
	textdomain (PACKAGE);

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);
	poptSetOtherOptionHelp (ctx, _("[OPTION...] file"));
	while (poptGetNextOpt (ctx) > 0);

	/* Any option? */
	if (argc <= 1) {
		poptPrintUsage (ctx, stdout, 0);
		return (0);
	}

	if (tag) {
		eo.tag = exif_tag_from_string (tag);
		if (!eo.tag || !exif_tag_get_name (eo.tag)) {
			fprintf (stderr, _("Invalid tag '%s'!"), tag);
			fprintf (stderr, "\n");
			return (1);
		}
	}

	if (show_description) {
		if (!eo.tag) {
			fprintf (stderr, _("Please specify a tag!"));
			fprintf (stderr, "\n");
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
				fprintf (stderr, "\n");
				exit (1);
			}
			
			if (list_tags) {
				action_tag_table (*args, ed);
			} else if (eo.tag)
				search_entry (ed, eo.tag);
			else if (extract_thumbnail) {

				/* No thumbnail? Exit. */
				if (!ed->data) {
					fprintf (stderr, _("'%s' does not "
						"contain a thumbnail!"),
						*args);
					fprintf (stderr, "\n");
					return (1);
				}

				/* Where to save the thumbnail? */
				if (output)
					strncpy (filename, output,
						 sizeof (filename));
				else {
					strncpy (filename, *args,
						 sizeof (filename));
					strncat (filename, ".thumb.jpeg",
						 sizeof (filename));
				}

				/* Save the thumbnail */
				f = fopen (filename, "wb");
				if (!f) {
					fprintf (stderr,
						_("Could not open '%s' for "
						"writing (%m)!"), filename);
					fprintf (stderr, "\n");
					return (1);
				}
				fwrite (ed->data, 1, ed->size, f);
				fclose (f);
				fprintf (stdout, _("Wrote file '%s'."),
					 filename);
				fprintf (stdout, "\n");

			} else if (ithumbnail) {

				/* Get rid of the old thumbnail */
				if (ed->data) {
					free (ed->data);
					ed->data = NULL;
				}
				ed->size = 0;

				f = fopen (ithumbnail, "rb");
				if (!f) {
					fprintf (stderr, _("Could not open "
						"'%s' (%m)!"), ithumbnail);
					fprintf (stderr, "\n");
					return (1);
				}
				fseek (f, 0, SEEK_END);
				ed->size = ftell (f);
				ed->data = malloc (sizeof (char) * ed->size);
				if (ed->size && !ed->data) {
					fprintf (stderr, _("Could not "
						"allocate %i byte(s)."),
						ed->size);
					fprintf (stderr, "\n");
					return (1);
				}
				fseek (f, 0, SEEK_SET);
				if (fread (ed->data, sizeof (char),
					   ed->size, f) != ed->size) {
					fprintf (stderr, _("Could not read "
						"'%s' (%m)."), ithumbnail);
					fprintf (stderr, "\n");
					return (1);
				}
				fclose (f);

				/* Where to save the resulting file? */
				if (output)
					strncpy (filename, output,
						 sizeof (filename));
				else {
					strncpy (filename, *args,
						 sizeof (filename));
					strncat (filename, ".modified.jpeg",
						 sizeof (filename));
				}

				/* Parse the JPEG file */
				jdata = jpeg_data_new_from_file (*args);
				if (!jdata) {
					fprintf (stderr,
						_("Could not parse JPEG file "
						"'%s'."), *args);
					fprintf (stderr, "\n");
					return (1);
				}

				jpeg_data_set_exif_data (jdata, ed);

				/* Save the modified image. */
				jpeg_data_save_file (jdata, filename);
				jpeg_data_unref (jdata);

				fprintf (stdout, _("Wrote file '%s'."),
					 filename);
				fprintf (stdout, "\n");
			} else
				action_tag_list (*args, ed, eo.use_ids);
			exif_data_unref (ed);
			args++;
		}
	}

	poptFreeContext (ctx);

	return (0);
}
