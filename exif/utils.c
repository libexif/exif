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
#include <stdio.h>

ExifTag
exif_tag_from_string (const char *string)
{
	ExifTag tag;
	unsigned int i, number;
	const char *name;

	if (!string)
		return 0xffff;

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
	for (tag = 0xffff; (int)tag > 0; tag--) {
		name = exif_tag_get_title (tag);
		if (name && !strcmp (string, name))
			return (tag);
	}
	return (0xffff);
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
