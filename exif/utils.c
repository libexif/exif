/* utils.c
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

#include "config.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

ExifTag
exif_tag_from_string (const char *string)
{
	ExifTag tag;
	unsigned int i, number;
	int t;
	const char *name;

	if (!string)
		return EXIF_INVALID_TAG;

	/* Is the string a decimal number? */
	if (strspn (string, "0123456789") == strlen (string))
		return (atoi (string));

	/* Is the string a hexadecimal number? */
	if (sscanf (string, "0x%x%n", &number, &i) == 1 && !string[i])
		return ((ExifTag)number);

	/* Is the string a tag's name? */
	if ((tag = exif_tag_from_name (string)) != 0)
		return (tag);

	/* Is the string a tag's title? */
	for (t = 0xffff; t >= 0; t--) {
		tag = (ExifTag) t;
		name = exif_tag_get_title (tag);
		if (name && !strcmp (string, name))
			return (tag);
	}
	return EXIF_INVALID_TAG;
}

ExifIfd
exif_ifd_from_string (const char *string)
{
	unsigned int i;

	if (!string)
		return (-1);

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		if (!strcmp (string, exif_ifd_get_name (i)))
			return (i);
	}

	return (-1);
}

/*! An alternative to exif_loader_get_data() that allows the options and
 * data type to be set.
 *
 * \param[in] loader the loader
 * \param[in] options one or more bitwise ORred options from ExifDataOption
 * \param[in] dt image data type
 * \return allocated ExifData
 */
ExifData *
exif_get_data_opts(ExifLoader *loader, ExifLog *log,
		   int options, ExifDataType dt)
{
	ExifData *ed;
	const unsigned char *buf;
	unsigned int buf_size;

	if (!loader || !log)
		return NULL;

	exif_loader_get_buf (loader, &buf, &buf_size);
	if (buf_size == 0)
		return NULL;
	ed = exif_data_new();
	exif_data_log (ed, log);

	/* Clear the slate before setting the requested options */
	exif_data_unset_option (ed, ~0);
	exif_data_set_option (ed, options);
	exif_data_set_data_type (ed, dt);
	exif_data_load_data (ed, buf, buf_size);

	return ed;
}

/*! Returns the number of bytes of data needed to display n characters of
 * the given multibyte string in the current locale character encoding.
 * Any multibyte conversion error is treated as the end of the string.
 * \param[in] mbs multibyte string
 * \param[in,out] len number of characters for which a count of bytes is
 * requested; on exit, returns the number of characters that are represented
 * by the returned number of bytes (this may be less but never more than the
 * value on entry)
 * \return number of bytes starting at mbs make up len characters
 * \note This can probably be rewritten using mbsrtowcs()
 */
#ifdef HAVE_MBLEN
size_t exif_mbstrlen(const char *mbs, size_t *len)
{
	int clen;
	size_t blen = 0, count = 0, maxlen = strlen(mbs);

	/* Iterate through the multibyte string one character at a time */
	while (*mbs && *len) {
		clen = mblen(mbs, maxlen);
		if (clen < 0)
			break;
		mbs += clen;
		blen += clen;	/* total bytes needed for string so far */
		--*len;
		++count;	/* number of characters in string so far */
		maxlen -= clen;
	}
	*len = count;
	return blen;
}
#else
/* Simple version that works only with single-byte-per-character encodings */
size_t exif_mbstrlen(const char *mbs, size_t *len)
{
	size_t clen = strlen(mbs);
	if (clen < *len)
		*len = clen;
	return *len;
}
#endif
