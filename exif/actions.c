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

	printf (_("EXIF tags in %-25.25s "), filename);
	printf ("%-8.8s", _("IFD 0"));
	printf ("%-8.8s", _("EXIF"));
	printf ("%-8.8s", _("GPS"));
	printf ("%-8.8s", _("IFD 1"));
	printf ("%-8.8s", _("Interop."));
	printf ("\n");
	for (tag = 0; tag < 0xffff; tag++) {
		name = exif_tag_get_name (tag);
		if (!name)
			continue;
		printf (_("  0x%04x %-29.29s"), tag, name);
		if (exif_content_get_entry (ed->ifd0, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		if (exif_content_get_entry (ed->ifd_exif, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		if (exif_content_get_entry (ed->ifd_gps, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		if (exif_content_get_entry (ed->ifd1, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		if (exif_content_get_entry (ed->ifd_interoperability, tag))
			printf (ENTRY_FOUND);
		else
			printf (ENTRY_NOT_FOUND);
		printf ("\n");
	}
}

static void
show_ifd (ExifContent *content, unsigned char ids)
{
        ExifEntry *e;
        unsigned int i;

        for (i = 0; i < content->count; i++) {
                e = content->entries[i];
                if (ids)
                        printf ("0x%04x", e->tag);
                else
                        printf ("%-20.20s", exif_tag_get_name (e->tag));
                printf ("|");
                if (ids)
                        printf ("%-73.73s", exif_entry_get_value (e));
                else
                        printf ("%-59.59s", exif_entry_get_value (e));
                printf ("\n");
        }
}

static void
print_hline (unsigned char ids)
{
        unsigned int i, width;

        width = (ids ? 6 : 20); 
        for (i = 0; i < width; i++)
                printf ("-");
        printf ("+");
        for (i = 0; i < 79 - width; i++)
                printf ("-");
        printf ("\n");
}

void
action_tag_list (const char *filename, ExifData *ed, unsigned char ids)
{
	printf (_("EXIF tags in '%s':\n"), filename);
	print_hline (ids);
        if (ids)
                printf ("%-6.6s", _("Tag"));
        else
                printf ("%-20.20s", _("Tag"));
        printf ("|");
        if (ids)
                printf ("%-73.73s", _("Value"));
        else
                printf ("%-59.59s", _("Value"));
        printf ("\n");
        print_hline (ids);
        if (ed->ifd0)
                show_ifd (ed->ifd0, ids);
        if (ed->ifd1)
                show_ifd (ed->ifd1, ids);
        if (ed->ifd_exif)
                show_ifd (ed->ifd_exif, ids);
        if (ed->ifd_gps)
                show_ifd (ed->ifd_gps, ids);
        if (ed->ifd_interoperability)
                show_ifd (ed->ifd_interoperability, ids);
        print_hline (ids);
        if (ed->size) {
                printf (_("EXIF data contains a thumbnail (%i bytes)."),
                        ed->size);
                printf ("\n");
        }
}
