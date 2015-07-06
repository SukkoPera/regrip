/* discedit.h
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

#ifndef GRIP_DISCEDIT_H
#define GRIP_DISCEDIT_H

#include "grip.h"

#define DEFAULT_GENRE "Other"


GtkWidget *MakeEditBox (GripInfo *ginfo);
void on_track_edit_changed (GtkWidget *widget, gpointer data);
void update_multi_artist (GtkWidget *widget, gpointer data);
void toggle_track_edit (GtkWidget *widget, gpointer data);
void set_title (GripInfo *ginfo, char *title);
void set_artist (GripInfo *ginfo, char *artist);
void set_year (GripInfo *ginfo, int year);
void set_genre (GripInfo *ginfo, char *genre);


#endif /* ifndef GRIP_DISCEDIT_H */
