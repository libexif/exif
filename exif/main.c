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
#include <string.h>
#include <popt.h>

#include <libexif/exif-data.h>

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

static void
show_ifd (ExifContent *content, unsigned char use_ids)
{
        ExifEntry *e;
        unsigned int i;

        for (i = 0; i < content->count; i++) {
                e = content->entries[i];
		if (use_ids)
			printf ("0x%04x", e->tag);
		else
			printf ("%-20.20s", exif_tag_get_name (e->tag));
                printf ("|");
		if (use_ids)
			printf ("%-73.73s", exif_entry_get_value (e));
		else
			printf ("%-59.59s", exif_entry_get_value (e));
                printf ("\n");
        }
}

static void
print_hline (unsigned char use_ids)
{
        unsigned int i, width;

	width = (use_ids ? 6 : 20);
        for (i = 0; i < width; i++)
                printf ("-");
        printf ("+");
        for (i = 0; i < 79 - width; i++)
                printf ("-");
        printf ("\n");
}

static void
show_exif (ExifData *ed, unsigned char use_ids)
{
	printf (_("EXIF tags:"));
        printf ("\n");
        print_hline (use_ids);
	if (use_ids)
		printf ("%-6.6s", _("Tag"));
	else
	        printf ("%-20.20s", _("Tag"));
        printf ("|");
	if (use_ids)
		printf ("%-73.73s", _("Value"));
	else
		printf ("%-59.59s", _("Value"));
        printf ("\n");
        print_hline (use_ids);
        if (ed->ifd0)
                show_ifd (ed->ifd0, use_ids);
        if (ed->ifd1)
                show_ifd (ed->ifd1, use_ids);
        if (ed->ifd_exif)
                show_ifd (ed->ifd_exif, use_ids);
        if (ed->ifd_gps)
                show_ifd (ed->ifd_gps, use_ids);
        if (ed->ifd_interoperability)
                show_ifd (ed->ifd_interoperability, use_ids);
        print_hline (use_ids);
        if (ed->size) {
                printf (_("EXIF data contains a thumbnail (%i bytes)."),
                        ed->size);
                printf ("\n");
	}
}

typedef struct _ExifOptions ExifOptions;
struct _ExifOptions {
	unsigned char use_ids;
	ExifTag tag;
};

static ExifTag get_tag (const char *string)
{
	unsigned int i, number = 0, factor;
	ExifTag tag;

	if (!string)
		return (0);

	if (!strlen (string))
		return (0);

	/*
	 * Scan for 'x' in string. If not found, we have to deal
	 * with a decimal number.
	 */
	for (i = 0; i < strlen (string); i++)
		if (string[i] == 'x')
			break;
	if (i == strlen (string))
		return (atoi (string));

	string += i + 1;
	if (strlen (string) > 4)
		return (0);

	tag = 0;
	for (i = strlen (string); i > 0; i--) {
		switch (string[i - 1]) {
		case '0':
			number = 0;
			break;
		case '1':
			number = 1;
			break;
		case '2':
			number = 2;
			break;
		case '3':
			number = 3;
			break;
		case '4':
			number = 4;
			break;
		case '5':
			number = 5;
			break;
		case '6':
			number = 6;
			break;
		case '7':
			number = 7;
			break;
		case '8':
			number = 8;
			break;
		case '9':
			number = 9;
			break;
		case 'a':
		case 'A':
			number = 10;
			break;
		case 'b':
		case 'B':
			number = 11;
			break;
		case 'c':
		case 'C':
			number = 12;
			break;
		case 'd':
		case 'D':
			number = 13;
			break;
		case 'e':
		case 'E':
			number = 14;
			break;
		case 'f':
		case 'F':
			number = 15;
			break;
		default:
			return (0);
		}
		factor = 1 << ((strlen (string) - i) * 4);
		tag += (number * factor);
	}

	return (tag);
}

int
main (int argc, const char **argv)
{
	unsigned char list_tags = 0, show_description = 0;
	ExifOptions eo = {0, 0};
	poptContext ctx;
	const char **args, *tag = NULL, *name;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"use-ids", 'i', POPT_ARG_NONE, &eo.use_ids, 0,
		 N_("Use IDs instead of tag names"), NULL},
		{"tag", 't', POPT_ARG_STRING, &tag, 0,
		 N_("Select tag"), N_("tag")},
		{"list-tags", 'l', POPT_ARG_NONE, &list_tags, 0,
		 N_("List all EXIF tags"), NULL},
		{"show-description", 's', POPT_ARG_NONE, &show_description, 0,
		 N_("Show description of tag"), NULL},
		POPT_TABLEEND};
	unsigned int i;
	ExifData *ed;

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);
	poptSetOtherOptionHelp (ctx, _("[OPTION...] file"));
	while (poptGetNextOpt (ctx) > 0);

	/* Any option? */
	if (argc <= 1) {
		poptPrintUsage (ctx, stdout, 0);
		return (0);
	}

	if (list_tags) {
		printf (_("Listing EXIF tags:\n"));
		for (i = 0; i < 0xffff; i++) {
			name = exif_tag_get_name (i);
			if (name)
				printf (_("  0x%04x '%s'\n"), i, name);
		}
	}

	if (tag) {
		if (!eo.use_ids) {
			for (i = 0; i < 0xffff; i++) {
				name = exif_tag_get_name (i);
				if (name && !strcmp (tag, name))
					break;
			}
			if (i < 0xffff)
				eo.tag = i;
			else {
				fprintf (stderr, ("Invalid tag '%s'!\n"), tag);
				return (1);
			}
		} else {
			eo.tag = get_tag (tag);
			if (!exif_tag_get_name (eo.tag)) {
				fprintf (stderr, _("Invalid tag 0x%04x!\n"),
					 eo.tag);
				return (1);
			}
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
			if (eo.tag)
				search_entry (ed, eo.tag);
			else
				show_exif (ed, eo.use_ids);
			exif_data_unref (ed);
			*args++;
		}
	}

	poptFreeContext (ctx);

	return (0);
}
