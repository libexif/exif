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


static void
show_entry (ExifEntry *entry, const char *caption)
{
	unsigned int i;

	printf (_("EXIF entry 0x%x ('%s') exists in '%s':\n"), entry->tag,
		exif_tag_get_name (entry->tag), caption);
	printf (_("  Format: '%s'\n"), exif_format_get_name (entry->format));
	printf (_("  Components: %i\n"), (int) entry->components);
	printf (_("  Value: '%s'\n"), exif_entry_get_value (entry));
	printf (_("  Data:"));
	for (i = 0; i < entry->size; i++) {
		if (!(i % 10))
			printf ("\n    ");
		printf ("0x%02x ", entry->data[i]);
	}
	printf ("\n");
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
}

typedef struct _ExifOptions ExifOptions;
struct _ExifOptions {
	unsigned char use_ids;
	ExifTag tag;
};

int
main (int argc, const char **argv)
{
	unsigned char list_tags = 0, show_description = 0;
	unsigned char extract_thumbnail = 0;
	ExifOptions eo = {0, 0};
	poptContext ctx;
	const char **args, *tag = NULL, *output = NULL;
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
		{"output", 'o', POPT_ARG_STRING, &output, 0,
		 N_("Output file"), N_("filename")},
		POPT_TABLEEND};
	ExifData *ed;
	char filename[1024];
	FILE *f;

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
		if (!eo.tag) {
			fprintf (stderr, ("Invalid tag '%s'!\n"), tag);
			return (1);
		}
	}

	if (show_description) {
		if (!eo.tag) {
			fprintf (stderr, _("Please specify a tag!\n"));
			return (1);
		}
		printf (_("Tag 0x%04x ('%s'): %s\n"), eo.tag,
			exif_tag_get_name (eo.tag),
			exif_tag_get_description (eo.tag));
		return (0);
	}

	args = poptGetArgs (ctx);

	if (args) {
		while (*args) {
			printf (_("Processing '%s'...\n"), *args);
			ed = exif_data_new_from_file (*args);
			if (list_tags) {
				action_tag_table (*args, ed);
			} else if (eo.tag)
				search_entry (ed, eo.tag);
			else if (extract_thumbnail) {

				/* No thumbnail? Exit. */
				if (!ed->data) {
					fprintf (stderr, N_("'%s' does not "
						"contain a thumbnail!"),
						*args);
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
					return (1);
				}
				fwrite (ed->data, 1, ed->size, f);
				fclose (f);
			} else 
				action_tag_list (*args, ed, eo.use_ids);
			exif_data_unref (ed);
			*args++;
		}
	}

	poptFreeContext (ctx);

	return (0);
}
