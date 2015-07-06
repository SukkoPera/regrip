/* tray.c
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

#include <glib.h>
#include <glib/gi18n.h>
#include "uihelper.h"
#include "cdplay.h"
#include "rip.h"
#include "tray.h"
#include "../pixmaps/rip1.xpm"
#include "../pixmaps/menuplay.xpm"
#include "../pixmaps/menupause.xpm"
#include "../pixmaps/menustop.xpm"
#include "../pixmaps/menuprev.xpm"
#include "../pixmaps/menunext.xpm"

static void make_tray_icon (GripInfo *ginfo);
static void on_play (GtkWidget *widget, gpointer data);
static void on_pause (GtkWidget *widget, gpointer data);
static void on_next (GtkWidget *widget, gpointer data);
static void on_prev (GtkWidget *widget, gpointer data);
static void on_stop (GtkWidget *widget, gpointer data);
static void on_rip_enc (GtkWidget *widget, gpointer data);
static void on_quit (GtkWidget *widget, gpointer data);
static gboolean tray_icon_button_press (GtkWidget *widget, GdkEventButton *event,
                                     gpointer data);

/*
 * update_tray
 *
 * Main functionality implemented here; handles:
 * - updating the tooltip
 * - if the tray icon is to be shown/built
 * - when to make menu sensitive/unsensitive
 * - NOTE: ginfo -> show_tray_icon is set in the Config  ->  Misc tab
 */

void update_tray (GripInfo *ginfo) {
	gchar *text, *riptext = NULL;
	gchar *artist = strlen (ginfo -> ddata.data_artist) > 0 ?
	                ginfo -> ddata.data_artist : _("Artist");
	gchar *title = strlen (ginfo -> ddata.data_title) > 0 ? ginfo -> ddata.data_title :
	               _("Title");
	GripGUI *uinfo = &(ginfo -> gui_info);

	int tmin, tsec, emin, esec;

	/* Decide if the tray icon is going to be displayed or not */
	if (ginfo -> show_tray_icon) {
		if (!ginfo -> tray_icon_made) {
			make_tray_icon (ginfo);
		}
	} else {
		if (uinfo -> tray_icon) {
			gtk_widget_destroy (GTK_WIDGET (uinfo -> tray_icon));
			uinfo -> tray_icon = NULL;
		}

		ginfo -> tray_icon_made = FALSE;
	}

	/* tray icon is present so we can make our tooltip */
	if (ginfo -> show_tray_icon) {
		if (ginfo -> playing) {
			tray_ungray_menu (ginfo);
			tmin = ginfo -> disc.track[ginfo -> current_track_index].length.mins;
			tsec = ginfo -> disc.track[ginfo -> current_track_index].length.secs;
			emin = ginfo -> disc.track_time.mins;
			esec = ginfo -> disc.track_time.secs;
			text = g_strdup_printf (_("%s - %s\n%02d:%02d of %02d:%02d"), artist,
			                        ginfo -> ddata.data_track[ginfo -> current_track_index].track_name, emin, esec,
			                        tmin, tsec);
		} else if (!ginfo -> playing) {
			if (ginfo -> ripping) {
				tray_gray_menu (ginfo);
				riptext = (ginfo -> ripping) ? g_strdup_printf (
				              _("Ripping Track %02d:\t%6.2f%% (%6.2f%%)"), ginfo -> rip_track + 1,
				              ginfo -> rip_percent * 100, ginfo -> rip_tot_percent * 100) : NULL;
				text = g_strdup_printf (_("%s - %s\n%s"), artist, title, riptext);
			} else {
				tray_ungray_menu (ginfo);
				text = g_strdup_printf (_("%s - %s\nIdle"), artist, title);
			}
		}

		gtk_tooltips_set_tip (GTK_TOOLTIPS (uinfo -> tray_tips), uinfo -> tray_ebox, text,
		                      NULL);

		if (riptext) {
			g_free (riptext);
		}

		g_free (text);
	}
}

/*
 * tray_menu_show_play/tray_menu_show_pause
 *
 * - set whether the Play menu item or Pause menu item is displayed
 */

void tray_menu_show_play (GripInfo *ginfo) {
	GripGUI *uinfo = &(ginfo -> gui_info);

	gtk_widget_hide (GTK_WIDGET (uinfo -> tray_menu_pause));
	gtk_widget_show (GTK_WIDGET (uinfo -> tray_menu_play));
}

void tray_menu_show_pause (GripInfo *ginfo) {
	GripGUI *uinfo = &(ginfo -> gui_info);

	gtk_widget_hide (GTK_WIDGET (uinfo -> tray_menu_play));
	gtk_widget_show (GTK_WIDGET (uinfo -> tray_menu_pause));
}

/*
 * tray_gray_menu/tray_ungray_menu
 *
 * - sets sensitivity of the menu items
 */

static void toggle_menu_item_sensitive (GtkWidget *widget, gpointer data) {
	gtk_widget_set_sensitive (GTK_WIDGET (widget), GPOINTER_TO_INT (data));
}

void tray_gray_menu (GripInfo *ginfo) {
	GripGUI *uinfo = &(ginfo -> gui_info);

	if (ginfo -> tray_menu_sensitive) {
		gtk_container_foreach (GTK_CONTAINER (uinfo -> tray_menu),
		                       toggle_menu_item_sensitive, GINT_TO_POINTER (FALSE));
		ginfo -> tray_menu_sensitive = FALSE;
	}
}

void tray_ungray_menu (GripInfo *ginfo) {
	GripGUI *uinfo = &(ginfo -> gui_info);

	if (!ginfo -> tray_menu_sensitive) {
		gtk_container_foreach (GTK_CONTAINER (uinfo -> tray_menu),
		                       toggle_menu_item_sensitive, GINT_TO_POINTER (TRUE));
		ginfo -> tray_menu_sensitive = TRUE;
	}
}

/*
 * make_tray_icon
 *
 * - tray icon and menu is built here
 * - NOTE: I added the function build_menuitem_xpm to uihelper.c
 */

static void make_tray_icon (GripInfo *ginfo) {
	GtkWidget *image, *mentry, *hb, *img;
	GripGUI *uinfo = &(ginfo -> gui_info);

	uinfo -> tray_icon = egg_tray_icon_new ("Grip");

	uinfo -> tray_ebox = gtk_event_box_new ();

	gtk_container_set_border_width (GTK_CONTAINER (uinfo -> tray_icon), 0);

	gtk_container_add (GTK_CONTAINER (uinfo -> tray_icon), uinfo -> tray_ebox);

	image = gtk_image_new_from_file (GNOME_ICONDIR "/griptray.png");

	gtk_container_add (GTK_CONTAINER (uinfo -> tray_ebox), image);

	uinfo -> tray_tips = gtk_tooltips_new ();

	uinfo -> tray_menu = gtk_menu_new ();

	img = (GtkWidget *) load_xpm (uinfo -> app, menuplay_xpm);
	uinfo -> tray_menu_play = (GtkWidget *)build_menuitem_xpm (img, _("Play"));
	g_signal_connect (uinfo -> tray_menu_play, "activate", G_CALLBACK (on_play),
	                  ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu),
	                       uinfo -> tray_menu_play);

	img = (GtkWidget *) load_xpm (uinfo -> app, menupause_xpm);
	uinfo -> tray_menu_pause = (GtkWidget *)build_menuitem_xpm (img, _("Pause"));
	g_signal_connect (uinfo -> tray_menu_pause, "activate", G_CALLBACK (on_pause),
	                  ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu),
	                       uinfo -> tray_menu_pause);

	img = (GtkWidget *) load_xpm (uinfo -> app, menustop_xpm);
	mentry = (GtkWidget *)build_menuitem_xpm (img, _("Stop"));
	g_signal_connect (mentry, "activate", G_CALLBACK (on_stop), ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), mentry);

	hb = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), hb);

	img = (GtkWidget *) load_xpm (uinfo -> app, menuprev_xpm);
	mentry = (GtkWidget *) build_menuitem_xpm (img, _("Previous"));
	g_signal_connect (mentry, "activate", G_CALLBACK (on_prev), ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), mentry);

	img = (GtkWidget *) load_xpm (uinfo -> app, menunext_xpm);
	mentry = (GtkWidget *) build_menuitem_xpm (img, _("Next"));
	g_signal_connect (mentry, "activate", G_CALLBACK (on_next), ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), mentry);

	hb = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), hb);

	img = (GtkWidget *) load_xpm (uinfo -> app, rip1_xpm);
	mentry = (GtkWidget *) build_menuitem_xpm (img, _("Rip and Encode"));
	g_signal_connect (mentry, "activate", G_CALLBACK (on_rip_enc), ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), mentry);

	hb = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), hb);

	mentry = (GtkWidget *) build_menuitem (GTK_STOCK_QUIT, _("Quit"), TRUE);
	g_signal_connect (mentry, "activate", G_CALLBACK (on_quit), ginfo);
	gtk_menu_shell_append (GTK_MENU_SHELL (uinfo -> tray_menu), mentry);

	g_signal_connect (uinfo -> tray_ebox, "button-press-event",
	                  G_CALLBACK (tray_icon_button_press), ginfo);

	gtk_widget_show_all (uinfo -> tray_menu);

	gtk_widget_hide (uinfo -> tray_menu_pause);

	gtk_widget_show_all (GTK_WIDGET (uinfo -> tray_icon));

	ginfo -> tray_icon_made = TRUE;
}

/*
 * tray_icon_button_press
 *
 * - handles the showing/hiding of the main window and displaying the menu
 * - NOTE: ginfo -> app_visible is set by on_app_window_state in grip.c
 */

static gboolean tray_icon_button_press (GtkWidget *widget, GdkEventButton *event,
                                     gpointer data) {
	GripInfo *ginfo = (GripInfo *) data;
	GripGUI *uinfo = &(ginfo -> gui_info);

	if (event -> button == 1) {
		if (ginfo -> app_visible) {
			gtk_window_get_position (GTK_WINDOW (uinfo -> app), &uinfo -> x, &uinfo -> y);
			gtk_widget_hide (GTK_WIDGET (uinfo -> app));
		} else {
			gtk_window_move (GTK_WINDOW (uinfo -> app), uinfo -> x, uinfo -> y);
			gtk_window_present (GTK_WINDOW (uinfo -> app));
		}

		return TRUE;
	}

	if (event -> button == 3) {
		gtk_menu_popup (GTK_MENU (uinfo -> tray_menu), NULL, NULL, NULL, NULL,
		                event -> button, event -> time);

		return TRUE;
	}

	return FALSE;
}

/*
 * Callbacks for the menu entries; pretty self-explanatory
 */

static void on_play (GtkWidget *widget, gpointer data) {
	on_play_track (NULL, data);
}

static void on_pause (GtkWidget *widget, gpointer data) {
	on_play_track (NULL, data);
}

static void on_stop (GtkWidget *widget, gpointer data) {
	Stopon_play (NULL, data);
}

static void on_next (GtkWidget *widget, gpointer data) {
	on_next_track (NULL, data);
}

static void on_prev (GtkWidget *widget, gpointer data) {
	on_prev_track (NULL, data);
}

static void on_rip_enc (GtkWidget *widget, gpointer data) {
	int i;
	GripInfo *ginfo = (GripInfo *) data;

	/* this gets rid of the annoying 'Rip Whole CD?' dialog box */
	for (i = 0; i < ginfo -> disc.num_tracks; i++) {
		set_checked (&(ginfo -> gui_info), i, TRUE);
	}

	do_rip (NULL, data);
}

static void on_quit (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo = (GripInfo *) data;
	GripGUI *uinfo = &(ginfo -> gui_info);

	grip_die (uinfo -> app, NULL);
}
