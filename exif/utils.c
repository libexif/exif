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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>

ExifTag
exif_tag_from_string (const char *string)
{
	ExifTag tag;
	unsigned int i, number, factor;
	const char *name;

	if (!string)
		return (0);

	/* Is the string a tag's name or title? */
	for (tag = 0xffff; tag > 0; tag--) {
		name = exif_tag_get_name (tag);
		if (name && !strcmp (string, name))
			return (tag);
		name = exif_tag_get_title (tag);
		if (name && !strcmp (string, name))
			return (tag);
	}

	/* Is the string a decimal number? */
	if (strspn (string, "0123456789") == strlen (string))
		return (atoi (string));

	/* Is the string a hexadecimal number? */
	for (i = 0; i < strlen (string); i++)
		if (string[i] == 'x')
			break;
	if (i == strlen (string))
		return (0);

	string += i + 1;
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

#ifdef HAVE_MNOTE

MNoteTag
mnote_tag_from_string (MNoteData *note, const char *string)
{
	MNoteTag tag;
	unsigned int i, number, factor;
	const char *name;

	if (!string)
		return (0);

	/* Is the string a tag's name or title? */
	for (tag = 0xffff; tag > 0; tag--) {
		name = mnote_tag_get_name (note, tag);
		if (name && !strcmp (string, name))
			return (tag);
		name = mnote_tag_get_title (note, tag);
		if (name && !strcmp (string, name))
			return (tag);
	}

	/* Is the string a decimal number? */
	if (strspn (string, "0123456789") == strlen (string))
		return (atoi (string));

	/* Is the string a hexadecimal number? */
	for (i = 0; i < strlen (string); i++)
		if (string[i] == 'x')
			break;
	if (i == strlen (string))
		return (0);

	string += i + 1;
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

#endif
