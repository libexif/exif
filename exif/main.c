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
	printf (_("EXIF entry 0x%x ('%s') exists in '%s':\n"), entry->tag,
		exif_tag_get_name (entry->tag), caption);
	printf (_("  Format: '%s'\n"), exif_format_get_name (entry->format));
	printf (_("  Components: %i\n"), (int) entry->components);
	printf (_("  Value: '%s'\n"), exif_entry_get_value (entry));
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
show_ifd (ExifContent *content)
{
        ExifEntry *e;
        unsigned int i;

        for (i = 0; i < content->count; i++) {
                e = content->entries[i];
                printf ("%-20.20s", exif_tag_get_name (e->tag));
                printf ("|");
                printf ("%-59.59s", exif_entry_get_value (e));
                printf ("\n");
        }
}

static void
print_hline (void)
{
        int i;

        for (i = 0; i < 20; i++)
                printf ("-");
        printf ("+");
        for (i = 0; i < 59; i++)
                printf ("-");
        printf ("\n");
}

static void
show_exif (ExifData *ed)
{
	printf (_("EXIF tags:"));
        printf ("\n");
        print_hline ();
        printf ("%-20.20s", _("Tag"));
        printf ("|");
        printf ("%-59.59s", _("Value"));
        printf ("\n");
        print_hline ();
        if (ed->ifd0)
                show_ifd (ed->ifd0);
        if (ed->ifd1)
                show_ifd (ed->ifd1);
        if (ed->ifd_exif)
                show_ifd (ed->ifd_exif);
        if (ed->ifd_gps)
                show_ifd (ed->ifd_gps);
        if (ed->ifd_interoperability)
                show_ifd (ed->ifd_interoperability);
        print_hline ();
        if (ed->size) {
                printf (_("EXIF data contains a thumbnail (%i bytes)."),
                        ed->size);
                printf ("\n");
	}
}

int
main (int argc, const char **argv)
{
	ExifTag tag = 0;
	poptContext ctx;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"tag", 't', POPT_ARG_INT, &tag, 0, N_("Select entry with tag"),
		 N_("tag")},
		POPT_TABLEEND};
	const char **args;
	ExifData *ed;

	ctx = poptGetContext (PACKAGE, argc, argv, options, 0);

	while (poptGetNextOpt (ctx) > 0);

	args = poptGetArgs (ctx);

	if (args) {
		while (*args) {
			printf (_("Processing '%s'...\n"), *args);
			ed = exif_data_new_from_file (*args);
			if (tag)
				search_entry (ed, tag);
			else
				show_exif (ed);
			exif_data_unref (ed);
			*args++;
		}
	}

	poptFreeContext (ctx);

	return (0);
}
