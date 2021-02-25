/* actions.c
 *
 * Copyright (c) 2002-2008 Lutz Mueller <lutz@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

#include "config.h"
#include "actions.h"
#include "exif-i18n.h"
#include "libjpeg/jpeg-data.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <libexif/exif-ifd.h>

#define CN(s) ((s) ? (s) : "(NULL)")

#define TAG_VALUE_BUF 1024

#define SEP " "

static void
convert_arg_to_entry (const char *set_value, ExifEntry *e, ExifByteOrder o, ExifLog *log)
{
	unsigned int i, numcomponents;
	char *value_p, *buf;

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
			/* TODO: get this from the current locale */
			memcpy ((char *) e->data, "ASCII\0\0\0", 8);
			memcpy ((char *) e->data + 8, set_value, 
				strlen (set_value));
		} else
			strcpy ((char *) e->data, set_value);
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

	/* Copy the string so we can modify it */
	buf = strdup(set_value);
	if (!buf) exit(1);
	value_p = strtok(buf, SEP);
	numcomponents = e->components;
	for (i = 0; i < numcomponents; ++i) {
		unsigned char s;

		if (!value_p) {
				fprintf (stderr, _("Too few components specified "
					  "(need %d, found %d)\n"), numcomponents, i);
				exit (1);
		}
		if (!isdigit(*value_p) && (*value_p != '+') && (*value_p != '-')) {
				fprintf (stderr, _("Numeric value expected\n"));
				exit (1);
		}

		s = exif_format_get_size (e->format);
		switch (e->format) {
		case EXIF_FORMAT_ASCII:
			exif_log (log, -1, "exif", _("Internal error. "
				"Please contact <%s>."), PACKAGE_BUGREPORT);
			break;
		case EXIF_FORMAT_SHORT:
			exif_set_short (e->data + (s * i), o, atoi (value_p));
			break;
		case EXIF_FORMAT_SSHORT:
			exif_set_sshort (e->data + (s * i), o, atoi (value_p));
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
			exif_set_long (e->data + (s * i), o, atol (value_p));
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
			exif_set_slong (e->data + (s * i), o, atol (value_p));
			break;
		case EXIF_FORMAT_BYTE:
		case EXIF_FORMAT_SBYTE:
		case EXIF_FORMAT_UNDEFINED: /* treat as byte array */
			e->data[s * i] = atoi (value_p);
			break;
		case EXIF_FORMAT_FLOAT:
		case EXIF_FORMAT_DOUBLE:
		default:
			fprintf (stderr, _("Not yet implemented!"));
			fputc ('\n', stderr);
			exit (1);
		}
		value_p = strtok(NULL, SEP);
	}
	free(buf);
	if (value_p) {
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
	if (jpeg_data_save_file (jdata, fout) == 0)
			exif_log (log, -1, "exif", _("Could not write "
				"'%s' (%s)."), fout, strerror (errno));
	jpeg_data_unref (jdata);

	fprintf (stdout, _("Wrote file '%s'."), fout);
	fprintf (stdout, "\n");
}

static void
show_entry (ExifEntry *entry, unsigned int machine_readable)
{
	ExifIfd ifd = exif_entry_get_ifd (entry);

	if (machine_readable) {
		char b[TAG_VALUE_BUF];

		fprintf (stdout, "%s\n", C(exif_entry_get_value (entry, b, sizeof (b))));
		return;
	}

	/*
	 * The C() macro can point to a static buffer so these printfs
	 * cannot be combined.
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

/*! If the entry doesn't exist, create it. */
ExifEntry *
action_create_value (ExifData *ed, ExifLog *log, ExifTag tag, ExifIfd ifd)
{
	ExifEntry *e;

	if (!((e = exif_content_get_entry (ed->ifd[ifd], tag)))) {
	    exif_log (log, EXIF_LOG_CODE_DEBUG, "exif", _("Adding entry..."));
	    e = exif_entry_new ();
	    exif_content_add_entry (ed->ifd[ifd], e);
	    exif_entry_initialize (e, tag);
	    /* The entry has been added to the IFD, so we can unref it */
	    exif_entry_unref(e);
	}
	return e;
}

void
action_set_value (ExifData *ed, ExifLog *log, ExifParams p)
{
	/* If the entry doesn't exist, create it. */
	ExifEntry *e = action_create_value(ed, log, p.tag, p.ifd);

	/* Now set the value and save the data. */
	convert_arg_to_entry (p.set_value, e, exif_data_get_byte_order (ed), log);
}

void
action_remove_tag (ExifData *ed, ExifLog *log, ExifParams p)
{
	ExifIfd ifd;
	ExifEntry *e;

	/* We do have 2 optional parameters: ifd and tag */
	if (p.tag == EXIF_INVALID_TAG && (p.ifd < EXIF_IFD_0 || p.ifd >= EXIF_IFD_COUNT))
		for (ifd = EXIF_IFD_0; ifd < EXIF_IFD_COUNT; ifd++)
			while (ed->ifd[ifd] && ed->ifd[ifd]->count)
				exif_content_remove_entry (ed->ifd[ifd],
					ed->ifd[ifd]->entries[0]);
	else if (p.tag == EXIF_INVALID_TAG)
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
	(void) log;  /* unused */
	(void) p;  /* unused */
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
	if (!f) {
		exif_log (log, -1, "exif", _("Could not open "
			"'%s' (%s)!"), p.set_thumb, strerror (errno));
	} else {
		long fsize;
		if (fseek (f, 0, SEEK_END) < 0) {
			fclose(f);
			exif_log (log, -1, "exif", _("Could not determine size of "
				"'%s' (%s)."), p.set_thumb, strerror (errno));
			return;
		}
		fsize = ftell (f);
		if (fsize < 0) {
			fclose(f);
			exif_log (log, -1, "exif", _("Could not determine size of "
				"'%s' (%s)."), p.set_thumb, strerror (errno));
			return;
		}
		ed->size = fsize;
		ed->data = malloc (sizeof (char) * ed->size);
		if (ed->size && !ed->data) {
			EXIF_LOG_NO_MEMORY (log, "exif", sizeof (char) * ed->size);
			exit (1);
		}
		if (fseek (f, 0, SEEK_SET) < 0) {
			fclose(f);
			exif_log (log, -1, "exif", _("Could not determine size of "
				"'%s' (%s)."), p.set_thumb, strerror (errno));
			return;
		}
		if (fread (ed->data, sizeof (char), ed->size, f) != ed->size)
			exif_log (log, -1, "exif", _("Could not read "
				"'%s' (%s)."), p.set_thumb, strerror (errno));
		if (fclose (f) < 0)
			exif_log (log, -1, "exif", _("Could not read "
				"'%s' (%s)."), p.set_thumb, strerror (errno));
	}
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
		exif_log (log, -1, "exif", _("Could not open '%s' for "
			"writing (%s)!"), fout, strerror (errno));
	else {
		if (fwrite (ed->data, 1, ed->size, f) != ed->size) {
			exif_log (log, -1, "exif", _("Could not write '%s' (%s)."),
				fout, strerror (errno));
		};
		if (fclose (f) == EOF)
			exif_log (log, -1, "exif", _("Could not write '%s' (%s)."),
				fout, strerror (errno));
		fprintf (stdout, _("Wrote file '%s'."), fout);
		fprintf (stdout, "\n");
	}
}

void
action_tag_table (ExifData *ed, ExifParams p)
{
	unsigned int tag;
	const char *name;
	char txt[TAG_VALUE_BUF];
	ExifIfd i;
	int fieldwidth, bytes;
	size_t width;

#define ENTRY_FOUND     "   *   "
#define ENTRY_NOT_FOUND "   -   "

	snprintf (txt, sizeof (txt) - 1, _("EXIF tags in '%s':"), p.fin);
	fieldwidth = width = p.width - 36;
	bytes = exif_mbstrlen(txt, &width);
	printf ("%.*s%*s", bytes, txt, fieldwidth-(int)width, "");

	for (i = (ExifIfd)0; i < EXIF_IFD_COUNT; i++) {
		int space;
		fieldwidth = width = 7;
		bytes = exif_mbstrlen(exif_ifd_get_name (i), &width);
		space = fieldwidth-width;
		printf ("%*s%.*s%*s", space/2, "", bytes, exif_ifd_get_name (i),
			space - space/2, "");
	}
	fputc ('\n', stdout);

	for (tag = 0; tag < 0xffff; tag++) {
		/*
		 * Display the name of the first tag of this number found.
		 * Since there is some overlap (e.g. with GPS tags), this
		 * name could sometimes be incorrect for the specific tags
		 * found in this file.
		 */
		name = exif_tag_get_title(tag);
		if (!name)
			continue;

		fieldwidth = width = p.width - 43;
		bytes = exif_mbstrlen(C(name), &width);
		printf ("0x%04x %.*s%*s",
			tag, bytes, C(name), fieldwidth-(int)width, "");
		for (i = (ExifIfd)0; i < EXIF_IFD_COUNT; i++)
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
	const ExifParams *p = data;
	char v[TAG_VALUE_BUF];
	ExifIfd ifd = exif_entry_get_ifd (e);
	const char *str;
	int fieldwidth, bytes;
	size_t width;

	if (p->use_ids)
		printf("0x%04x", e->tag);
	else {
		str = C(exif_tag_get_title_in_ifd (e->tag, ifd));
		fieldwidth = width = 20;
		bytes = exif_mbstrlen(str, &width);
		printf ("%.*s%*s", bytes, str, fieldwidth-(int)width, "");
	}
	printf ("|");

	fieldwidth = width = p->use_ids ? p->width-8 : p->width-22;
	str = C(exif_entry_get_value (e, v, sizeof(v)));
	bytes = exif_mbstrlen(str, &width);
	printf("%.*s", bytes, str);
	fputc ('\n', stdout);
}

static void
show_ifd (ExifContent *content, void *data)
{
	exif_content_foreach_entry (content, show_entry_list, data);
}

static void
print_hline (unsigned int ids, unsigned int screenwidth)
{
        unsigned int i, width;

        width = ids ? 6 : 20; 
        for (i = 0; i < width; i++)
		fputc ('-', stdout);
        fputc ('+', stdout);
        for (i = 0; i < screenwidth - 2 - width; i++)
		fputc ('-', stdout);
	fputc ('\n', stdout);
}

void
action_mnote_list (ExifData *ed, ExifParams p)
{
	char b[TAG_VALUE_BUF], b1[TAG_VALUE_BUF], b2[TAG_VALUE_BUF];
	unsigned int i, c, id;
	ExifMnoteData *n;
	const char *s;
	int fieldwidth, bytes;
	size_t width;

	n = exif_data_get_mnote_data (ed);
	if (!n) {
		printf (_("Unknown format or nonexistent MakerNote.\n"));
		return;
	}

	c = exif_mnote_data_count (n);
	if (!p.machine_readable) {
		switch (c) {
		case 0:
			printf (_("MakerNote does not contain any value.\n"));
			break;
		default:
			printf (ngettext("MakerNote contains %i value:\n",
					 "MakerNote contains %i values:\n",
					 c), c);
		}
	}
	for (i = 0; i < c; i++) {
	        if (p.use_ids) {
			id = exif_mnote_data_get_id  (n,i);
			sprintf(b1,"0x%04x",id);
		} else {
			s = C (exif_mnote_data_get_title (n, i));
			strncpy (b1, s && *s ? s : _("Unknown Tag"), TAG_VALUE_BUF);
			b1[sizeof(b1)-1] = 0;
		}
		if (p.machine_readable) {
			printf ("%s\t", b1);
		} else {
			fieldwidth = width = p.use_ids ? 6 : 20;
			bytes = exif_mbstrlen(b1, &width);
			printf ("%.*s%*s|", bytes, b1, fieldwidth-(int)width, "");
		}

		s = C (exif_mnote_data_get_value (n, i, b, TAG_VALUE_BUF));
		strncpy (b2, s ? s : _("Unknown value"), TAG_VALUE_BUF);
		b2[sizeof(b2)-1] = 0;
        	if (p.use_ids) {
			fputs (b2, stdout);
        	} else {
			fieldwidth = width = p.width-22;
			bytes = exif_mbstrlen(b2, &width);
			printf ("%.*s", bytes, b2);
		}
        	fputc ('\n', stdout);
	}
}

void
action_tag_list (ExifData *ed, ExifParams p)
{
	ExifByteOrder order;
	const char *s;
	int fieldwidth, bytes;
	size_t width;

	if (!ed)
		return;

	order = exif_data_get_byte_order (ed);
	printf (_("EXIF tags in '%s' ('%s' byte order):"), p.fin,
		exif_byte_order_get_name (order));
	fputc ('\n', stdout);
	print_hline (p.use_ids, p.width);

	fieldwidth = width = p.use_ids ? 6 : 20;
	s = _("Tag");
	bytes = exif_mbstrlen(s, &width);
	printf ("%.*s%*s", bytes, s, fieldwidth-(int)width, "");
	fputc ('|', stdout);

	fieldwidth = width = p.use_ids ? p.width-8 : p.width-22;
	s = _("Value");
	bytes = exif_mbstrlen(s, &width);
	printf ("%.*s", bytes, s);
        fputc ('\n', stdout);
        print_hline (p.use_ids, p.width);

	if (p.ifd < EXIF_IFD_COUNT)
		/* Show only a single IFD */
		show_ifd(ed->ifd[p.ifd], &p);
	else
		/* Show contents of all IFDs */
		exif_data_foreach_content (ed, show_ifd, &p);

        print_hline (p.use_ids, p.width);
        if (ed->size) {
                printf (_("EXIF data contains a thumbnail "
			  "(%i bytes)."), ed->size);
                fputc ('\n', stdout);
        }
}

static void
show_entry_machine (ExifEntry *e, void *data)
{
	unsigned int *ids = data;
	char v[TAG_VALUE_BUF];
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

	if (p.ifd < EXIF_IFD_COUNT)
		/* Show only a single IFD */
		show_ifd_machine(ed->ifd[p.ifd], &p.use_ids);
	else
		/* Show contents of all IFDs */
		exif_data_foreach_content (ed, show_ifd_machine, &p.use_ids);

	if (ed->size)
		fprintf (stdout, _("ThumbnailSize\t%i\n"), ed->size);
}

/*!
 * Replace characters which are invalid in an XML tag with safe characters
 * in place.
 */
static inline void
remove_bad_chars(char *s)
{
	while (*s) {
		if ((*s == '(') || (*s == ')') || (*s == ' '))
			*s = '_';
		++s;
	}
}

/*!
 * Escape any special XML characters in the text and return a new static string
 * buffer.
 */
static const char *
escape_xml(const char *text)
{
	static char *escaped;
	static size_t escaped_size;
	char *out;
	size_t len;

	if (!strlen(text)) return "empty string";

	for (out=escaped, len=0; *text; ++len, ++out, ++text) {
		/* Make sure there's plenty of room for a quoted character */
		if ((len + 8) > escaped_size) {
			char *bigger_escaped;
			escaped_size += 128;
			bigger_escaped = realloc(escaped, escaped_size);
			if (!bigger_escaped) {
				free(escaped);	/* avoid leaking memory */
				escaped = NULL;
				escaped_size = 0;
				/* Error string is cleverly chosen to fail XML validation */
				return ">>> out of memory <<<";
			}
			out = bigger_escaped + len;
			escaped = bigger_escaped;
		}
		switch (*text) {
			case '&':
				strcpy(out, "&amp;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			case '<':
				strcpy(out, "&lt;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			case '>':
				strcpy(out, "&gt;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			default:
				*out = *text;
				break;
		}
	}
	*out = '\x0';  /* NUL terminate the string */
	return escaped;
}

static void
show_entry_xml (ExifEntry *e, void *data)
{
	unsigned char *ids = data;
	char v[TAG_VALUE_BUF], t[TAG_VALUE_BUF];

	if (*ids) {
		fprintf (stdout, "<x%04x>", e->tag);
		fprintf (stdout, "%s", escape_xml(exif_entry_get_value (e, v, sizeof (v))));
		fprintf (stdout, "</x%04x>", e->tag);
	} else {
		strncpy (t, exif_tag_get_title_in_ifd(e->tag, exif_entry_get_ifd(e)), sizeof (t));
		t[sizeof(t)-1] = 0;

		/* Remove invalid characters from tag eg. (, ), space */
		remove_bad_chars(t);

		fprintf (stdout, "\t<%s>", t);
		fprintf (stdout, "%s", escape_xml(exif_entry_get_value (e, v, sizeof (v))));
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
	if (p.ifd < EXIF_IFD_COUNT)
		/* Show only a single IFD */
		show_xml(ed->ifd[p.ifd], &p.use_ids);
	else
		/* Show contents of all IFDs */
		exif_data_foreach_content (ed, show_xml, &p.use_ids);
	fprintf(stdout, "</exif>\n");
}
