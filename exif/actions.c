/* actions.c
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
#include "actions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libexif/exif-ifd.h>

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

#define ENTRY_FOUND     "   *   "
#define ENTRY_NOT_FOUND "   -   "

void
action_tag_table (const char *filename, ExifData *ed)
{
	unsigned int tag;
	const char *name;
	char txt[1024];
	unsigned int i;

	memset (txt, 0, sizeof (txt));
	snprintf (txt, sizeof (txt) - 1, _("EXIF tags in '%s':"), filename);
	fprintf (stdout, "%-38.38s", txt);
	for (i = 0; i < EXIF_IFD_COUNT; i++)
		fprintf (stdout, "%-7.7s", exif_ifd_get_name (i));
	fputc ('\n', stdout);
	for (tag = 0; tag < 0xffff; tag++) {
		name = exif_tag_get_title (tag);
		if (!name)
			continue;
		fprintf (stdout, "  0x%04x %-29.29s", tag, name);
		for (i = 0; i < EXIF_IFD_COUNT; i++)
			if (exif_content_get_entry (ed->ifd[i], tag))
				printf (ENTRY_FOUND);
			else
				printf (ENTRY_NOT_FOUND);
		fputc ('\n', stdout);
	}
}

void
action_ntag_table (const char *filename, MNoteData *en)
{
	unsigned int tag;
	const char *name;
	char txt[1024];

	memset (txt, 0, sizeof (txt));
	snprintf (txt, sizeof (txt) - 1, _("EXIF MakerNote tags in '%s':"), filename);
	fprintf (stdout, "%-38.38s", txt);
	fprintf (stdout, "%-7.7s", "MakerNote");
	fputc ('\n', stdout);
	for (tag = 0; tag < 0xffff; tag++) {
		name = mnote_tag_get_title (en, tag);
		if (!name)
			continue;
		fprintf (stdout, "  0x%04x %-29.29s", tag, name);
		if (mnote_data_get_value (en, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		fputc ('\n', stdout);
	}
}

static void
show_entry (ExifEntry *entry, void *data)
{
	unsigned char *ids = data;

	if (*ids)
		fprintf (stdout, "0x%04x", entry->tag);
	else
		fprintf (stdout, "%-20.20s", exif_tag_get_title (entry->tag));
	printf ("|");
	if (*ids)
		fprintf (stdout, "%-72.72s", exif_entry_get_value (entry));
	else
		fprintf (stdout, "%-58.58s", exif_entry_get_value (entry));
	fputc ('\n', stdout);
}

static void
show_ifd (ExifContent *content, void *data)
{
	exif_content_foreach_entry (content, show_entry, data);
}

static void
show_note_entry (MNoteData *note, MNoteTag tag, void *data)
{
	unsigned char *ids = data;

	if (*ids)
		fprintf (stdout, "0x%04x", tag);
	else
		fprintf (stdout, "%-20.20s", mnote_tag_get_title (note, tag));
	printf ("|");
	if (*ids)
		fprintf (stdout, "%-72.72s", mnote_data_get_value (note, tag));
	else
		fprintf (stdout, "%-58.58s", mnote_data_get_value (note, tag));
	fputc ('\n', stdout);
}

static void
print_hline (unsigned char ids)
{
        unsigned int i, width;

        width = (ids ? 6 : 20); 
        for (i = 0; i < width; i++)
                fputc ('-', stdout);
        fputc ('+', stdout);
        for (i = 0; i < 78 - width; i++)
		fputc ('-', stdout);
	fputc ('\n', stdout);
}

void
action_tag_list (const char *filename, ExifData *ed, unsigned char ids)
{
	ExifByteOrder order;

	if (!ed)
		return;

	order = exif_data_get_byte_order (ed);
	fprintf (stdout, _("EXIF tags in '%s' ('%s' byte order):"), filename,
		exif_byte_order_get_name (order));
	fputc ('\n', stdout);
	print_hline (ids);
        if (ids)
                fprintf (stdout, "%-6.6s", _("Tag"));
        else
                fprintf (stdout, "%-20.20s", _("Tag"));
	fputc ('|', stdout);
        if (ids)
		fprintf (stdout, "%-72.72s", _("Value"));
        else
                fprintf (stdout, "%-58.58s", _("Value"));
        fputc ('\n', stdout);
        print_hline (ids);
	exif_data_foreach_content (ed, show_ifd, &ids);
        print_hline (ids);
        if (ed->size) {
                fprintf (stdout, _("EXIF data contains a thumbnail "
				   "(%i bytes)."), ed->size);
                fputc ('\n', stdout);
        }
}

void
action_ntag_list (const char *filename, MNoteData *en, unsigned char ids)
{
	ExifByteOrder order;

	if (!en)
		return;

	order = mnote_data_get_byte_order (en);
	fprintf (stdout, _("EXIF MakerNote tags in '%s' ('%s' byte order):"), filename,
		exif_byte_order_get_name (order));
	fputc ('\n', stdout);
	print_hline (ids);
        if (ids)
                fprintf (stdout, "%-6.6s", _("Tag"));
        else
                fprintf (stdout, "%-20.20s", _("Tag"));
	fputc ('|', stdout);
        if (ids)
		fprintf (stdout, "%-72.72s", _("Value"));
        else
                fprintf (stdout, "%-58.58s", _("Value"));
        fputc ('\n', stdout);
        print_hline (ids);
	mnote_data_foreach_entry (en, show_note_entry, &ids);
        print_hline (ids);
}
