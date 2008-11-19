/* actions.c
 *
 * Copyright © 2002-2008 Lutz Müller <lutz@users.sourceforge.net>
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
#include "libjpeg/jpeg-data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libexif/exif-ifd.h>

#define ENTRY_FOUND     "   *   "
#define ENTRY_NOT_FOUND "   -   "

#define CN(s) ((s) ? (s) : "(NULL)")

static void
convert_arg_to_entry (const char *set_value, ExifEntry *e, ExifByteOrder o, ExifLog *log)
{
	unsigned int i, numcomponents;
	char *value_p;

        /*
	 * ASCII strings are handled separately,
	 * since they don't require any conversion.
	 */
        if (e->format == EXIF_FORMAT_ASCII ||
	    e->tag == EXIF_TAG_USER_COMMENT) {
		if (e->data) free (e->data);
		e->components = strlen (set_value) + 1;
		if (e->tag == EXIF_TAG_USER_COMMENT)
			e->components += 8 - 1;
		e->size = sizeof (char) * e->components;
		e->data = malloc (e->size);
                if (!e->data) {
                        fprintf (stderr, _("Not enough memory."));
                        fputc ('\n', stderr);
                        exit (1);
                }
		if (e->tag == EXIF_TAG_USER_COMMENT) {
			/* assume ASCII charset */
			memcpy ((char *) e->data, "ASCII\0\0\0", 8);
			memcpy ((char *) e->data + 8, (char *) set_value, 
				strlen (set_value));
		} else
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
			exif_log (log, -1, "exif", _("Internal error. "
				"Please contact <%s>."), PACKAGE_BUGREPORT);
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

void
action_save (ExifData *ed, ExifLog *log, ExifParams p, const char *fout)
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
		if (ds > 0xffff)
			exif_log (log, -1, "exif", _("Too much EXIF data "
				"(%i bytes). Only %i bytes are allowed."),
				ds, 0xffff);
	};

	jpeg_data_set_exif_data (jdata, ed);

	/* Save the modified image. */
	jpeg_data_save_file (jdata, fout);
	jpeg_data_unref (jdata);

	fprintf (stdout, _("Wrote file '%s'."), fout);
	fprintf (stdout, "\n");
}

static void
show_entry (ExifEntry *entry, unsigned int machine_readable)
{
	ExifIfd ifd = exif_entry_get_ifd (entry);

	if (machine_readable) {
		char b[1024];

		fprintf (stdout, "%s\n", C(exif_entry_get_value (entry, b, sizeof (b))));
		return;
	}

	/*
	 * The C() macro can point to a static buffer so these printfs
	 * must be done separately.
	 */
	printf (_("EXIF entry '%s' "),
		C(exif_tag_get_title_in_ifd (entry->tag, ifd)));
	printf (_("(0x%x, '%s') "),
		entry->tag,
		C(exif_tag_get_name_in_ifd (entry->tag, ifd)));
	printf (_("exists in IFD '%s':\n"),
		C(exif_ifd_get_name (ifd)));

	exif_entry_dump (entry, 0);
}

void
action_set_value (ExifData *ed, ExifLog *log, ExifParams p)
{
	ExifEntry *e;

	/* If the entry doesn't exist, create it. */
	if (!((e = exif_content_get_entry (ed->ifd[p.ifd], p.tag)))) {
	    exif_log (log, EXIF_LOG_CODE_DEBUG, "exif", "Adding entry...");
	    e = exif_entry_new ();
	    exif_content_add_entry (ed->ifd[p.ifd], e);
	    exif_entry_initialize (e, p.tag);
	}

	/* Now set the value and save the data. */
	convert_arg_to_entry (p.set_value, e, exif_data_get_byte_order (ed), log);
}

void
action_remove_tag (ExifData *ed, ExifLog *log, ExifParams p)
{
	ExifIfd ifd;
	ExifEntry *e;

	/* We do have 2 optional parameters: ifd and tag */
	if (!p.tag && (p.ifd < EXIF_IFD_0 || p.ifd >= EXIF_IFD_COUNT))
		for (ifd = EXIF_IFD_0; ifd < EXIF_IFD_COUNT; ifd++)
			while (ed->ifd[ifd] && ed->ifd[ifd]->count)
				exif_content_remove_entry (ed->ifd[ifd],
					ed->ifd[ifd]->entries[0]);
	else if (!p.tag)
		while (ed->ifd[p.ifd] && ed->ifd[p.ifd]->count)
			exif_content_remove_entry (ed->ifd[p.ifd],
				ed->ifd[p.ifd]->entries[0]);
	else if (p.ifd < EXIF_IFD_0 || p.ifd >= EXIF_IFD_COUNT)
		while ((e = exif_data_get_entry (ed, p.tag)))
			exif_content_remove_entry (e->parent, e);
	else if (!((e = exif_content_get_entry (ed->ifd[p.ifd], p.tag))))
		exif_log (log, -1, "exif", _("IFD '%s' does not contain a "
			"tag '%s'!"), exif_ifd_get_name (p.ifd),
			exif_tag_get_name_in_ifd (p.tag, p.ifd));
	else
		exif_content_remove_entry (ed->ifd[p.ifd], e);
}

void
action_remove_thumb (ExifData *ed, ExifLog *log, ExifParams p)
{
	if (ed->data) {
		free (ed->data);
		ed->data = NULL;
	}
	ed->size = 0;
}

void
action_insert_thumb (ExifData *ed, ExifLog *log, ExifParams p)
{
	FILE *f;

	if (!ed) return;

	/* Get rid of the thumbnail */
	action_remove_thumb (ed, log, p);

	/* Insert new thumbnail */
	f = fopen (p.set_thumb, "rb");
	if (!f)
#ifdef __GNUC__
		exif_log (log, -1, "exif", _("Could not open "
			"'%s' (%m)!"), p.set_thumb);
#else
		exif_log (log, -1, "exif", _("Could not open "
			"'%s' (%s)!"), p.set_thumb, strerror (errno));
#endif
	fseek (f, 0, SEEK_END);
	ed->size = ftell (f);
	ed->data = malloc (sizeof (char) * ed->size);
	if (ed->size && !ed->data) EXIF_LOG_NO_MEMORY (log, "exif", ed->size);
	fseek (f, 0, SEEK_SET);
	if (fread (ed->data, sizeof (char), ed->size, f) != ed->size)
#ifdef __GNUC__
		exif_log (log, -1, "exif", _("Could not read "
			"'%s' (%m)."), p.set_thumb);
#else
		exif_log (log, -1, "exif", _("Could not read "
			"'%s' (%s)."), p.set_thumb, strerror (errno));
#endif
	fclose (f);
}

void
action_show_tag (ExifData *ed, ExifLog *log, ExifParams p)
{
	ExifEntry *e;
	unsigned int i;

	if (!ed) return;

	/* We have one optional parameter: ifd */
	if ((p.ifd >= EXIF_IFD_0) && (p.ifd < EXIF_IFD_COUNT)) {
		if ((e = exif_content_get_entry (ed->ifd[p.ifd], p.tag)))
			show_entry (e, p.machine_readable);
		else
			exif_log (log, -1, "exif", _("IFD '%s' "
				"does not contain tag '%s'."),
					exif_ifd_get_name (p.ifd),
					exif_tag_get_name (p.tag));
	} else {
		if (!exif_data_get_entry (ed, p.tag))
			exif_log (log, -1, "exif", _("'%s' does not contain "
				"tag '%s'."), p.fin,
				exif_tag_get_name (p.tag));
		else for (i = 0; i < EXIF_IFD_COUNT; i++)
			if ((e = exif_content_get_entry (ed->ifd[i], p.tag)))
				show_entry (e, p.machine_readable);
	}
}

void
action_save_thumb (ExifData *ed, ExifLog *log, ExifParams p, const char *fout)
{
	FILE *f;

	if (!ed) return;

	/* No thumbnail? Exit. */
	if (!ed->data) {
		exif_log (log, -1, "exif", _("'%s' does not "
			"contain a thumbnail!"), p.fin);
		return;
	}

	/* Save the thumbnail */
	f = fopen (fout, "wb");
	if (!f)
#ifdef __GNUC__
	exif_log (log, -1, "exif", _("Could not open '%s' for "
		"writing (%m)!"), fout);
#else
	exif_log (log, -1, "exif", _("Could not open '%s' for "
		"writing (%s)!"), fout, strerror (errno));
#endif
	fwrite (ed->data, 1, ed->size, f);
	fclose (f);
	fprintf (stdout, _("Wrote file '%s'."), fout);
	fprintf (stdout, "\n");
}

void
action_tag_table (ExifData *ed, ExifParams p)
{
	unsigned int tag;
	const char *name;
	char txt[1024];
	unsigned int i;

	memset (txt, 0, sizeof (txt));
	snprintf (txt, sizeof (txt) - 1, _("EXIF tags in '%s':"), p.fin);
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
show_entry_list (ExifEntry *e, void *data)
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
	exif_content_foreach_entry (content, show_entry_list, data);
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
action_mnote_list (ExifData *ed, ExifParams p)
{
	unsigned int i, bs = 1024, c, id;
	char b[1024];
	char b1[1024], b2[1024];
	ExifMnoteData *n;
	const char *s;

	n = exif_data_get_mnote_data (ed);
	if (!n) {
		printf (_("Unknown format or nonexistent MakerNote.\n"));
		return;
	}

	c = exif_mnote_data_count (n);
	switch (c) {
	case 0:
		printf (_("MakerNote does not contain any value.\n"));
		break;
	default:
		printf (ngettext("MakerNote contains %i value:\n",
			         "MakerNote contains %i values:\n",
			 	 c), c);
	}
	for (i = 0; i < c; i++) {
	        if (p.use_ids) {
			id = exif_mnote_data_get_id  (n,i);
			sprintf(b1,"0x%04x",id);
		} else {
			s = C (exif_mnote_data_get_title (n, i));
			strncpy (b1, s && *s ? s : _("Unknown tag"), bs);
			b1[sizeof(b1)-1] = 0;
		}
		s = C (exif_mnote_data_get_value (n, i, b, bs));
		strncpy (b2, s ? s : _("Unknown value"), bs);
		b2[sizeof(b2)-1] = 0;
		/* printf ("%s|%s\n", b1, b2); */
        	if (p.use_ids)
                	fprintf (stdout, "%-6.6s", b1);
        	else
                	fprintf (stdout, "%-20.20s", b1);
		fputc ('|', stdout);
        	if (p.use_ids) {
			fputs (b2, stdout);
        	} else {
                	fprintf (stdout, "%-58.58s", b2);
		}
        	fputc ('\n', stdout);
	}
}

void
action_tag_list (ExifData *ed, ExifParams p)
{
	ExifByteOrder order;

	if (!ed)
		return;

	order = exif_data_get_byte_order (ed);
	fprintf (stdout, _("EXIF tags in '%s' ('%s' byte order):"), p.fin,
		exif_byte_order_get_name (order));
	fputc ('\n', stdout);
	print_hline (p.use_ids);
        if (p.use_ids) {
                fprintf (stdout, "%-6.6s", _("Tag"));
        } else {
                fprintf (stdout, "%-20.20s", _("Tag"));
        }
	fputc ('|', stdout);
        if (p.use_ids)
		fprintf (stdout, "%-72.72s", _("Value"));
        else
                fprintf (stdout, "%-58.58s", _("Value"));
        fputc ('\n', stdout);
        print_hline (p.use_ids);
	exif_data_foreach_content (ed, show_ifd, &p.use_ids);
        print_hline (p.use_ids);
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
action_tag_list_machine (ExifData *ed, ExifParams p)
{
	if (!ed) return;

	exif_data_foreach_content (ed, show_ifd_machine, &p.use_ids);
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
action_tag_list_xml (ExifData *ed, ExifParams p)
{
	if (!ed) return;

	fprintf(stdout, "<exif>\n");
	exif_data_foreach_content (ed, show_xml, &p.use_ids);
	fprintf(stdout, "</exif>\n");
}
