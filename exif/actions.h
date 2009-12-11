/* actions.h
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

#ifndef __ACTIONS_H__
#define __ACTIONS_H__

#include <libexif/exif-data.h>

typedef struct {
	ExifTag tag;
	ExifIfd ifd;

	unsigned int machine_readable;
	unsigned int use_ids;
	unsigned int width;

	const char *fin;

	char *set_value;
	char *set_thumb;
} ExifParams;

void action_insert_thumb     (ExifData *, ExifLog *, ExifParams);
void action_remove_thumb     (ExifData *, ExifLog *, ExifParams);
void action_show_tag         (ExifData *, ExifLog *, ExifParams);
void action_set_value        (ExifData *, ExifLog *, ExifParams);
void action_remove_tag       (ExifData *, ExifLog *, ExifParams);
ExifEntry *
action_create_value (ExifData *ed, ExifLog *log, ExifTag tag, ExifIfd ifd);

void action_save             (ExifData *, ExifLog *, ExifParams, const char *);
void action_save_thumb       (ExifData *, ExifLog *, ExifParams, const char *);

void action_tag_table        (ExifData *, ExifParams);
void action_tag_list         (ExifData *, ExifParams);
void action_tag_list_machine (ExifData *, ExifParams);
void action_tag_list_xml     (ExifData *, ExifParams);

void action_mnote_list       (ExifData *, ExifParams);

#endif /* __ACTIONS_H__ */
