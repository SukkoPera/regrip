/* cdplay.h
 *
 * Copyright (c) 1998-2015 Mike Oliphant <contact@nostatic.org>
 * Copyright (c) 2014-2015 SukkoPera <software@sukkology.net>
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

#ifndef GRIP_CDPLAY_H
#define GRIP_CDPLAY_H

#include "grip.h"

/* Time display modes */
#define TIME_MODE_TRACK 0
#define TIME_MODE_DISC 1
#define TIME_MODE_LEFT_TRACK 2
#define TIME_MODE_LEFT_DISC 3

/* Play mode types */
#define PM_NORMAL 0
#define PM_SHUFFLE 1
#define PM_PLAYLIST 2
#define PM_LASTMODE 3

/* Some shortcuts */
#define PREV_TRACK (ginfo->tracks_prog[ginfo->current_track_index - 1])
#define CURRENT_TRACK (ginfo->tracks_prog[ginfo->current_track_index])
#define NEXT_TRACK (ginfo->tracks_prog[ginfo->current_track_index + 1])


void min_max (GtkWidget *widget, gpointer data);
void set_current_track_index (GripInfo *ginfo, int track);
void set_checked (GripGUI *uinfo, int track, gboolean checked);
gboolean is_track_checked (GripGUI *uinfo, int track);
void eject_disc (GtkWidget *widget, gpointer data);
void play_segment (GripInfo *ginfo, int track);
void fast_fwd_disc (GripInfo *ginfo);
void rewind_disc (GripInfo *ginfo);
void lookup_disc (GripInfo *ginfo);
GtkWidget *MakePlayOpts (GripInfo *ginfo);
GtkWidget *MakeControls (GripInfo *ginfo);
int get_length_rip_width (GripInfo *ginfo);
void resize_track_list (GripInfo *ginfo);
GtkWidget *MakeTrackPage (GripInfo *ginfo);
void next_track (GripInfo *ginfo);
void check_for_new_disc (GripInfo *ginfo, gboolean force);
void scan_disc (GtkWidget *widget, gpointer data);
void update_display (GripInfo *ginfo);
void update_tracks (GripInfo *ginfo);
void submit_entry (GtkDialog *dialog, gint reply, gpointer data);

void on_play_track (GtkWidget *widget, gpointer data);
void Stopon_play (GtkWidget *widget, gpointer data);
void on_next_track (GtkWidget *widget, gpointer data);
void on_prev_track (GtkWidget *widget, gpointer data);

#endif /* ifndef GRIP_CDPLAY_H */
