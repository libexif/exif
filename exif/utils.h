/* utils.h
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-loader.h>

enum {EXIF_INVALID_TAG = 0xffff};

ExifTag exif_tag_from_string (const char *string);
ExifIfd exif_ifd_from_string (const char *string);
ExifData *exif_get_data_opts(ExifLoader *loader, ExifLog *log, int options, ExifDataType dt);
size_t exif_mbstrlen(const char *mbs, size_t *len);

#endif /* __UTILS_H__ */
