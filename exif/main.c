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

#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>

#ifdef HAVE_MNOTE
#include <libmnote/mnote-data.h>
#endif

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
show_entry (ExifEntry *entry, const char *caption)
{
	printf (_("EXIF entry '%s' (0x%x, '%s') exists in IFD '%s':"),
		exif_tag_get_title (entry->tag), entry->tag,
		exif_tag_get_name (entry->tag), caption);
	printf ("\n");

	exif_entry_dump(entry, 0);
}

#ifdef HAVE_MNOTE

static void
show_note_entry (MNoteData *note, MNoteTag tag)
{
	printf (_("MakerNote entry '%s' (0x%x, '%s'):"),
		mnote_tag_get_title (note, tag), tag,
		mnote_tag_get_name (note, tag));
	printf ("\n");

	mnote_data_dump_entry(note, tag, 0);
}

#endif

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
	unsigned int use_ids;
	ExifTag tag;
#ifdef HAVE_MNOTE
	MNoteTag ntag;
#endif
};

/*
 * Static variables. I had them first in main (), but people
 * compiling exif on IRIX complained about that not being compatible
 * with the "SGI MIPSpro C compiler". I don't understand and still think
 * these variables belong into main ().
 */
static unsigned int list_tags = 0, show_description = 0;
static unsigned int extract_thumbnail = 0, remove_thumbnail = 0;
static unsigned int remove_tag = 0;
#ifdef HAVE_MNOTE
static unsigned int list_ntags = 0;
#endif
static const char *set_value = NULL, *ifd_string = NULL, *tag_string = NULL;
#ifdef HAVE_MNOTE
static const char *ntag_string = NULL;
#endif
static ExifIfd ifd = -1;
static ExifTag tag = 0;
#ifdef HAVE_MNOTE
static MNoteTag ntag = 0;
static ExifOptions eo = {0, 0, 0};
#else
static ExifOptions eo = {0, 0};
#endif

int
main (int argc, const char **argv)
{
	/* POPT_ARG_NONE needs an int, not char! */
	poptContext ctx;
	const char **args, *output = NULL;
	const char *ithumbnail = NULL;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"ids", 'i', POPT_ARG_NONE, &eo.use_ids, 0,
		 N_("Show IDs instead of tag names"), NULL},
		{"tag", 't', POPT_ARG_STRING, &tag_string, 0,
		 N_("Select tag"), N_("tag")},
#ifdef HAVE_MNOTE
		{"ntag", '\0', POPT_ARG_STRING, &ntag_string, 0,
		 N_("Select MakerNote tag"), N_("ntag")},
#endif
		{"ifd", '\0', POPT_ARG_STRING, &ifd_string, 0,
		 N_("Select IFD"), N_("IFD")},
		{"list-tags", 'l', POPT_ARG_NONE, &list_tags, 0,
		 N_("List all EXIF tags"), NULL},
#ifdef HAVE_MNOTE
		{"list-ntags", '\0', POPT_ARG_NONE, &list_ntags, 0,
		 N_("List all EXIF MakerNote tags"), NULL},
#endif
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
		POPT_TABLEEND};
	ExifData *ed;
	ExifEntry *e;
#ifdef HAVE_MNOTE
	MNoteData *md = 0;
#endif
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
		poptPrintHelp (ctx, stdout, 0);
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
		if ((ifd < EXIF_IFD_0) || (ifd >= EXIF_IFD_COUNT) ||
		    !exif_ifd_get_name (ifd)) {
			fprintf (stderr, _("Invalid IFD '%s'. Valid IFDs are "
				"'0', '1', 'EXIF', 'GPS', and "
				"'Interoperability'."), ifd_string);
			fputc ('\n', stderr);
			return (1);
		}
	}

	if (show_description
#ifdef HAVE_MNOTE
	    && !ntag_string
#endif
	    ) {
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

#ifdef HAVE_MNOTE
			if (ntag_string || list_ntags || ntag) {
				e = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF],
							   EXIF_TAG_MAKER_NOTE);
				if (!e) {
					fprintf (stderr, _("'%s' does not "
							   "contain EXIF MakerNote data!"), *args);
					fputc ('\n', stderr);
					exit (1);
				}

				md = mnote_data_new_from_data (e->data, e->size);
				if (!md) {
					fprintf (stderr, "Could not parse EXIF tag MakerNote in '%s'!", *args);
					fputc ('\n', stderr);
					exit (1);
				}
			}
#endif

			/* Where do we save the output? */
			memset (fname, 0, sizeof (fname));
			if (output)
				strncpy (fname, output, sizeof (fname) - 1);
			else {
				strncpy (fname, *args, sizeof (fname) - 1);
				strncat (fname, ".modified.jpeg",
					 sizeof (fname) - 1);
			}

#ifdef HAVE_MNOTE
			if (ntag_string) {
				ntag = mnote_tag_from_string (md, ntag_string);
				if (!ntag || !mnote_tag_get_name (md, ntag)) {
					fprintf (stderr, _("Invalid MakerNote tag '%s'!"), ntag_string);
					fputc ('\n', stderr);
					return (1);
				}
				eo.ntag = ntag;

			}

			if (show_description && ntag_string) {
				if (!eo.ntag) {
					fprintf (stderr, _("Please specify a MakerNote tag!"));
					fputc ('\n', stderr);
					return (1);
				}
				printf (_("Tag '%s' (0x%04x, '%s'): %s"),
					mnote_tag_get_title (md, eo.ntag), eo.ntag,
					mnote_tag_get_name (md, eo.ntag),
					mnote_tag_get_description (md, eo.ntag));
				printf ("\n");
				return (0);
			}
#endif

			if (list_tags) {
				action_tag_table (*args, ed);
#ifdef HAVE_MNOTE
			} else if (list_ntags) {
				action_ntag_table (*args, md);
#endif
			} else if (tag && !set_value) {
				if ((ifd >= EXIF_IFD_0) &&
				    (ifd < EXIF_IFD_COUNT)) {
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
#ifdef HAVE_MNOTE
			} else if (ntag && !set_value) {
				char *value = mnote_data_get_value (md, ntag);
				if (value)
					show_note_entry (md, ntag);
				else {
					fprintf (stderr, _("Makernote "
						"does not contain tag "
						"'%s'."), 
						ntag_string);
					fputc ('\n', stderr);
					return (1);
				}
#endif
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

				/* Save the new data. */
				save_exif_data_to_file (ed, *args, fname);

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
				if ((ifd < EXIF_IFD_0) ||
				    (ifd >= EXIF_IFD_COUNT)) {
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
			e->components = strlen(e->data) + 1;
			i = e->components - 1;
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
			} else if (remove_tag) {
				
				/* We need an IFD. */
				if ((ifd < EXIF_IFD_0) ||
				    (ifd >= EXIF_IFD_COUNT)) {
					fprintf (stderr, _("You need to "
						 "specify an IFD!"));
					fputc ('\n', stderr);
					return (1);
				}

				if (!tag) {
					while (ed->ifd[ifd] &&
					       ed->ifd[ifd]->count)
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

				/* Save modified data. */
				save_exif_data_to_file (ed, *args, fname);

			} else
				action_tag_list (*args, ed, eo.use_ids);
			exif_data_unref (ed);
			args++;
		}
	} else
		poptPrintHelp (ctx, stdout, 0);

	poptFreeContext (ctx);

	return (0);
}
