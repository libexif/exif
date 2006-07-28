/* actions.c
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
#include "actions.h"
#include "exif-i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libexif/exif-ifd.h>

#define ENTRY_FOUND     "   *   "
#define ENTRY_NOT_FOUND "   -   "

#define CN(s) ((s) ? (s) : "(NULL)")

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
		fprintf (stdout, "  0x%04x %-29.29s", tag, C(name));
		for (i = 0; i < EXIF_IFD_COUNT; i++)
			if (exif_content_get_entry (ed->ifd[i], tag))
				printf (ENTRY_FOUND);
			else
				printf (ENTRY_NOT_FOUND);
		fputc ('\n', stdout);
	}
}

static void
show_entry (ExifEntry *e, void *data)
{
	unsigned char *ids = data;
	char v[128];
	ExifIfd ifd = exif_entry_get_ifd (e);

	if (*ids)
		fprintf (stdout, "0x%04x", e->tag);
	else
		fprintf (stdout, "%-20.20s", C(exif_tag_get_title_in_ifd (e->tag, ifd)));
	printf ("|");
	if (*ids)
		fprintf (stdout, "%-72.72s",
			 C(exif_entry_get_value (e, v, 73)));
	else
		fprintf (stdout, "%-58.58s",
			 C(exif_entry_get_value (e, v, 59)));
	fputc ('\n', stdout);
}

static void
show_ifd (ExifContent *content, void *data)
{
	exif_content_foreach_entry (content, show_entry, data);
}

static void
print_hline (unsigned char ids)
{
        unsigned int i, width;

        width = (ids ? 6 : 20); 
        for (i = 0; i < width; i++) fputc ('-', stdout);
        fputc ('+', stdout);
        for (i = 0; i < 78 - width; i++) fputc ('-', stdout);
	fputc ('\n', stdout);
}

void
action_mnote_list (const char *filename, ExifData *ed, unsigned char ids)
{
	unsigned int i, bs = 1024, c, id;
	char b[1024];
	char b1[1024], b2[1024];
	ExifMnoteData *n;
	const char *p;

	n = exif_data_get_mnote_data (ed);
	if (!n) {
		printf (_("Unknown MakerNote format.\n"));
		return;
	}

	c = exif_mnote_data_count (n);
	switch (c) {
	case 0:
		printf (_("MakerNote does not contain any value.\n"));
		break;
	case 1:
		printf (_("MakerNote contains 1 value:\n"));
		break;
	default:
		printf (_("MakerNote contains %i values:\n"), c);
	}
	for (i = 0; i < c; i++) {
	        if ( ids ) {
			id = exif_mnote_data_get_id  (n,i);
			sprintf(b1,"0x%04x",id);
		} else {
			p = C (exif_mnote_data_get_title (n, i));
			strncpy (b1, p ? p : _("Unknown tag"), bs);
		}
		p = C (exif_mnote_data_get_value (n, i, b, bs));
		strncpy (b2, p ? p : _("Unknown value"), bs);
		/* printf ("%s|%s\n", b1, b2); */
        	if (ids)
                	fprintf (stdout, "%-6.6s", b1);
        	else
                	fprintf (stdout, "%-20.20s", b1);
		fputc ('|', stdout);
        	if (ids) {
			fputs (CN (b2), stdout);
        	} else {
                	fprintf (stdout, "%-58.58s", b2);
		}
        	fputc ('\n', stdout);
	}
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
        if (ids) {
                fprintf (stdout, "%-6.6s", _("Tag"));
        } else {
                fprintf (stdout, "%-20.20s", _("Tag"));
        }
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

static void
show_entry_machine (ExifEntry *e, void *data)
{
	unsigned char *ids = data;
	char v[1024];
	ExifIfd ifd = exif_entry_get_ifd (e);

	if (*ids) {
		fprintf (stdout, "0x%04x", e->tag);
	} else {
		fputs (CN (exif_tag_get_title_in_ifd (e->tag, ifd)), stdout);
	}
	fputc ('\t', stdout);
	fputs (CN (exif_entry_get_value (e, v, sizeof (v))), stdout);
	fputc ('\n', stdout);
}

static void
show_ifd_machine (ExifContent *content, void *data)
{
	exif_content_foreach_entry (content, show_entry_machine, data);
}

void
action_tag_list_machine (const char *filename, ExifData *ed, unsigned char ids)
{
	if (!ed) return;

	exif_data_foreach_content (ed, show_ifd_machine, &ids);
	if (ed->size)
		fprintf (stdout, _("ThumbnailSize\t%i\n"), ed->size);
}

static void
show_entry_xml (ExifEntry *e, void *data)
{
	unsigned char *ids = data;
	char v[1024], t[1024];

	if (*ids) {
		fprintf (stdout, "<0x%04x>", e->tag);
		fprintf (stdout, "%s", exif_entry_get_value (e, v, sizeof (v)));
		fprintf (stdout, "</0x%04x>", e->tag);
	} else {
		int x;
		strncpy (t, exif_tag_get_title (e->tag), sizeof (t));

    /* Remove invalid characters from tag eg. (, ), space */
		for (x = 0; x < strlen (t); x++)
			if ((t[x] == '(') || (t[x] == ')') || (t[x] == ' '))
				t[x] = '_';

		fprintf (stdout, "\t<%s>", t);
		fprintf (stdout, "%s", exif_entry_get_value (e, v, sizeof (v)));
		fprintf (stdout, "</%s>\n", t);
	}
}

static void
show_xml (ExifContent *content, void *data)
{
	exif_content_foreach_entry (content, show_entry_xml, data);
}

void
action_tag_list_xml (const char *filename, ExifData *ed, unsigned char ids)
{
	if (!ed) return;

	fprintf(stdout, "<exif>\n");
	exif_data_foreach_content (ed, show_xml, &ids);
	fprintf(stdout, "</exif>\n");
}
