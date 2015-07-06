/* uihelper.h
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* Routines from uihelper.c */
GtkTooltips *make_tooltip (void);
GdkColor *make_color (int red, int green, int blue);
GtkStyle *make_style (GdkColor *fg, GdkColor *bg, gboolean do_grade);
GtkWidget *build_menuitem_xpm (GtkWidget *xpm, gchar *text);
GtkWidget *build_menuitem (gchar *impath, gchar *text, gboolean stock);
GtkWidget *new_blank_pixmap (GtkWidget *widget);
GtkWidget *image_button (GtkWidget *widget, GtkWidget *image);
GtkWidget *load_xpm (GtkWidget *widget, char **xpm);
void copy_pixmap (GtkPixmap *src, GtkPixmap *dest);
gint SizeInDubs (GdkFont *font, gint numchars);
void update_gtk (void);
