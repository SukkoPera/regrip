/* rip.h
 *
 * Copyright (c) 1998-2002  Mike Oliphant <oliphant@gtk.org>
 *
 *   https://github.com/SukkoPera/regrip
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GRIP_RIP_H
#define GRIP_RIP_H

#include "common.h"

/* Encode list structure */
typedef struct _encode_track {
	GripInfo *ginfo;
	int track_num;
	int start_frame;
	int end_frame;
	char song_name[MAX_STRING];
	char song_artist[MAX_STRING];
	char disc_name[MAX_STRING];
	char disc_artist[MAX_STRING];
    char wav_filename[MAX_STRING];
//  char mp3_filename[MAX_STRING];
	int song_year;
	char genre[MAX_STRING];
	int mins;
	int secs;
	int discid;
	double track_gain_adjustment;
	double disc_gain_adjustment;
} EncodeTrack;


void make_rip_page (GripInfo *ginfo);
unsigned long long bytes_left_in_fs (char *path);
char *FindRoot (char *str);
char *MakePath (char *str);
void kill_rip (GtkWidget *widget, gpointer data);
void update_rip_progress (GripInfo *ginfo);
char *TranslateSwitch (char switch_char, void *data, gboolean *munge);
void do_rip (GtkWidget *widget, gpointer data);
void fill_in_track_info (GripInfo *ginfo, int track, EncodeTrack *new_track);
void on_menuitem_rip_partial_activate (GtkMenuItem *menuitem, gpointer user_data);

#endif /* ifndef GRIP_RIP_H */
