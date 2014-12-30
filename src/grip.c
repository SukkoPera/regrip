/* grip.c
 *
 * Copyright (c) 1998-2004  Mike Oliphant <grip@nostatic.org>
 *
 *   http://sourceforge.net/projects/grip/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <config.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
//#include <gdk/gdkx.h>
//#include <X11/Xlib.h>
#include <time.h>
#include "grip.h"
#include <glib.h>
#include <glib/gi18n.h>
#include "discdb.h"
#include "cdplay.h"
#include "discedit.h"
#include "rip.h"
#include "uihelper.h"
#include "dialog.h"
#include "gripcfg.h"
#include "xpm.h"
#include "tray.h"

static gboolean gripDieOnWinCloseCB (GtkWidget *widget, GdkEvent *event,
                                     gpointer data);
static void ReallyDie (GtkDialog *dialog, gint reply, gpointer data);
static void MakeStatusPage (GripInfo *ginfo);
static void DoHelp (GtkWidget *widget, gpointer data);
static void MakeHelpPage (GripInfo *ginfo);
static void MakeAboutPage (GripGUI *uinfo);
static void MakeStyles (GripGUI *uinfo);
static void Homepage (void);
static void LoadImages (GripGUI *uinfo);
static void set_initial_config (GripInfo *ginfo);


static gboolean AppWindowStateCB (GtkWidget *widget, GdkEventWindowState *event,
                           gpointer data) {
	GripInfo *ginfo = (GripInfo *)data;
	GripGUI *uinfo = & (ginfo -> gui_info);
	GdkWindowState state = event -> new_window_state;

	if ((state & GDK_WINDOW_STATE_WITHDRAWN)
	        || (state & GDK_WINDOW_STATE_ICONIFIED)) {
		ginfo -> app_visible = FALSE;
		return TRUE;
	} else {
		ginfo -> app_visible = TRUE;
		gtk_window_get_position (GTK_WINDOW (uinfo -> app), &uinfo -> x, &uinfo -> y);
		return TRUE;
	}

	return FALSE;
}

static gboolean on_window_resize (GtkWindow *window, GdkEvent *event, gpointer user_data) {
    g_assert (event -> type == GDK_CONFIGURE);

    GdkEventConfigure *e = (GdkEventConfigure *) event;
    GripInfo *ginfo = (GripInfo *) user_data;

    g_debug ("New window size: %dx%d", e -> width, e -> height);
    g_debug ("New window position: %dx%d", e -> x, e -> y);

    g_settings_set_uint (ginfo -> settings, "win-width", e -> width);
    g_settings_set_uint (ginfo -> settings, "win-height", e -> height);
    // FIXME: Also save position

    // FALSE propagates event further
    return FALSE;
}

GtkWidget *GripNew (const gchar *geometry, char *device, char *scsi_device,
                    char *config_filename,
                    gboolean force_small,
                    gboolean local_mode, gboolean no_redirect) {
	GtkWidget *app;
	GripInfo *ginfo;
	GripGUI *uinfo;
	char buf[256];

	GError *err = NULL;
	gchar *icon_file = g_build_filename (GNOME_ICONDIR, "gripicon.png", NULL);

	if (!gtk_window_set_default_icon_from_file (icon_file, &err)) {
		gchar *msg = g_strdup_printf (_("Error: Unable to set window icon: %s"),
		                              err -> message);
		show_error (NULL, msg);
		g_free (msg);
	}

	g_free (icon_file);

	// Init global object
	ginfo = g_new0 (GripInfo, 1);

	app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (app), _("Regrip"));
	gtk_widget_add_events(app, GDK_CONFIGURE);
	g_signal_connect (app, "configure-event", G_CALLBACK (on_window_resize), ginfo);
	gtk_object_set_user_data (GTK_OBJECT (app), (gpointer) ginfo);

    // First of all, load settings
    ginfo -> settings = g_settings_new ("net.sukkology.software.regrip");
    ginfo -> settings_cdplay = g_settings_get_child (ginfo -> settings, "cdplay");
    ginfo -> settings_cdparanoia = g_settings_get_child (ginfo -> settings, "cdparanoia");
    ginfo -> settings_rip = g_settings_get_child (ginfo -> settings, "rip");
    ginfo -> settings_encoder = g_settings_get_child (ginfo -> settings, "encoder");
    ginfo -> settings_tag = g_settings_get_child (ginfo -> settings, "tag");
    ginfo -> settings_discdb = g_settings_get_child (ginfo -> settings, "discdb");
    ginfo -> settings_proxy = g_settings_get_child (ginfo -> settings, "proxy");

	uinfo = &(ginfo -> gui_info);
	uinfo -> app = app;
	uinfo -> status_window = NULL;
	uinfo -> rip_status_window = NULL;
	uinfo -> encode_status_window = NULL;
	uinfo -> track_list = NULL;

	uinfo -> win_width = g_settings_get_uint (ginfo -> settings, "win-width");
	uinfo -> win_height = g_settings_get_uint (ginfo -> settings, "win-height");
	uinfo -> win_height_edit = g_settings_get_uint (ginfo -> settings, "win-height-edit");
	uinfo -> win_width_min = g_settings_get_uint (ginfo -> settings, "win-width-min");
	uinfo -> win_height_min = g_settings_get_uint (ginfo -> settings, "win-height-min");

//	if (config_filename && *config_filename) {
//		g_snprintf (ginfo -> config_filename, 256, "%s", config_filename);
//	} else {
//		strcpy (ginfo -> config_filename, ".grip");
//	}

//	g_debug ("Using config file [%s]", ginfo -> config_filename);

	set_initial_config (ginfo);

	if (device) {
		g_snprintf (ginfo -> cd_device, 256, "%s", device);
	}

	if (scsi_device) {
		g_snprintf (ginfo -> force_scsi, 256, "%s", scsi_device);
	}

	uinfo -> minimized = force_small;
	ginfo -> local_mode = local_mode;
	ginfo -> do_redirect = !no_redirect;

	if (!CDInitDevice (ginfo -> cd_device, & (ginfo -> disc))) {
		sprintf (buf, _("Error: Unable to initialize [%s]\n"), ginfo -> cd_device);

		show_error (ginfo -> gui_info.app, buf);
	}

	CDStat (&(ginfo -> disc), TRUE);

	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (app), "regrip", "Regrip");
	g_signal_connect (G_OBJECT (app), "delete_event",
	                  G_CALLBACK (gripDieOnWinCloseCB), ginfo);

	if (uinfo -> minimized) {
		gtk_widget_set_size_request (GTK_WIDGET (app), uinfo -> win_width_min,
		                             uinfo -> win_height_min);

		gtk_window_resize (GTK_WINDOW (app), uinfo -> win_width_min,
		                   uinfo -> win_height_min);
	} else {
		gtk_widget_set_size_request (GTK_WIDGET (app), uinfo -> win_width,
		                             uinfo -> win_height);

		if (uinfo -> track_edit_visible) {
			gtk_window_resize (GTK_WINDOW (app), uinfo -> win_width,
			                   uinfo -> win_height_edit);
		} else {
			gtk_window_resize (GTK_WINDOW (app), uinfo -> win_width,
			                   uinfo -> win_height);
		}
	}

	gtk_widget_realize (app);

	uinfo -> winbox = gtk_vbox_new (FALSE, 3);

	if (!uinfo -> minimized) {
		gtk_container_border_width (GTK_CONTAINER (uinfo -> winbox), 3);
	}

	uinfo -> notebook = gtk_notebook_new ();

	LoadImages (uinfo);
	MakeStyles (uinfo);
	MakeTrackPage (ginfo);
	MakeRipPage (ginfo);
	MakeConfigPage (ginfo);
	MakeStatusPage (ginfo);
	MakeHelpPage (ginfo);
	MakeAboutPage (uinfo);
	ginfo -> tray_icon_made = FALSE;
	ginfo -> tray_menu_sensitive = TRUE;

	gtk_box_pack_start (GTK_BOX (uinfo -> winbox), uinfo -> notebook, TRUE, TRUE, 0);

	if (!uinfo -> minimized) {
		gtk_widget_show (uinfo -> notebook);
	}

	uinfo -> track_edit_box = MakeEditBox (ginfo);
	gtk_box_pack_start (GTK_BOX (uinfo -> winbox), uinfo -> track_edit_box,
	                    FALSE, FALSE, 0);

	if (uinfo -> track_edit_visible) {
		gtk_widget_show (uinfo -> track_edit_box);
	}


	uinfo -> playopts = MakePlayOpts (ginfo);
	gtk_box_pack_start (GTK_BOX (uinfo -> winbox), uinfo -> playopts, FALSE, FALSE, 0);

	if (uinfo -> track_prog_visible) {
		gtk_widget_show (uinfo -> playopts);
	}

	uinfo -> controls = MakeControls (ginfo);

	if (uinfo -> minimized) {
		gtk_box_pack_start (GTK_BOX (uinfo -> winbox), uinfo -> controls, TRUE, TRUE, 0);
	} else {
		gtk_box_pack_start (GTK_BOX (uinfo -> winbox), uinfo -> controls, FALSE, FALSE, 0);
	}

	gtk_widget_show (uinfo -> controls);

	gtk_container_add (GTK_CONTAINER (app), uinfo -> winbox);
	gtk_widget_show (uinfo -> winbox);

	CheckNewDisc (ginfo, FALSE);

	/* Check if we're running this version for the first time */
	if (strcmp (VERSION, ginfo -> version) != 0) {
		strcpy (ginfo -> version, VERSION);

		/* Check if we have a dev release */
		if (strlen (VERSION) >= 3 && strcmp (VERSION + strlen (VERSION) - 3, "svn") == 0) {
			show_warning (ginfo -> gui_info.app,
			              _("This is a development version of Regrip. If you encounter problems, you are encouraged to revert to the latest stable version."));
		}
	}

	g_signal_connect (app, "window-state-event", G_CALLBACK (AppWindowStateCB),
	                  ginfo);

	LogStatus (ginfo, _("Regrip started successfully\n"));

	return app;
}

static gboolean gripDieOnWinCloseCB (GtkWidget *widget, GdkEvent *event,
                                     gpointer data) {
	GripDie (widget, data);

	return TRUE;
}

void GripDie (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

#ifndef GRIPCD

	if (ginfo -> ripping) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (ginfo -> gui_info.app),
		                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                    GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
		                    _("Ripping is in progress.\nReally shut down?"));
        gtk_window_set_title (GTK_WINDOW (dialog), "Rip in progress");
		g_signal_connect (dialog,
		                  "response",
		                  G_CALLBACK (ReallyDie),
		                  ginfo);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		ReallyDie (NULL, GTK_RESPONSE_YES, ginfo);
	}

#else
	ReallyDie (NULL, GTK_RESPONSE_YES, ginfo);
#endif
}

static void ReallyDie (GtkDialog *dialog, gint reply, gpointer data) {
	GripInfo *ginfo;

	if (reply != GTK_RESPONSE_YES) {
		return;
	}

	ginfo = (GripInfo *)data;

#ifndef GRIPCD
	if (ginfo -> ripping) {
		KillRip (NULL, ginfo);
	}
#endif

	if (!ginfo -> no_interrupt) {
		CDStop (& (ginfo -> disc));
	}

	gtk_main_quit ();
}

GtkWidget *MakeNewPage (GtkWidget *notebook, char *name) {
	GtkWidget *page;
	GtkWidget *label;

	page = gtk_frame_new (NULL);
	gtk_widget_show (page);

	label = gtk_label_new (name);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	return page;
}

static void MakeStatusPage (GripInfo *ginfo) {
	GtkWidget *status_page;
	GtkWidget *vbox, *vbox2;
	GtkWidget *notebook;
	GtkWidget *page;
	GtkWidget *label;

	status_page = MakeNewPage (ginfo -> gui_info.notebook, _("Status"));

	vbox2 = gtk_vbox_new (FALSE, 0);
	notebook = gtk_notebook_new();

	page = gtk_frame_new (NULL);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	ginfo -> gui_info.status_window = NewStatusWindow (vbox);

	gtk_container_add (GTK_CONTAINER (page), vbox);
	gtk_widget_show (vbox);

	label = gtk_label_new (_("General"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
	gtk_widget_show (page);


	page = gtk_frame_new (NULL);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	ginfo -> gui_info.rip_status_window = NewStatusWindow (vbox);

	gtk_container_add (GTK_CONTAINER (page), vbox);
	gtk_widget_show (vbox);

	label = gtk_label_new (_("Rip"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
	/*  gtk_widget_show(page);*/


	page = gtk_frame_new (NULL);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	ginfo -> gui_info.encode_status_window = NewStatusWindow (vbox);

	gtk_container_add (GTK_CONTAINER (page), vbox);
	gtk_widget_show (vbox);

	label = gtk_label_new (_("Encode"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
	/*  gtk_widget_show(page);*/


	gtk_box_pack_start (GTK_BOX (vbox2), notebook, TRUE, TRUE, 0);
	gtk_widget_show (notebook);

	gtk_container_add (GTK_CONTAINER (status_page), vbox2);
	gtk_widget_show (vbox2);
}

void LogStatus (GripInfo *ginfo, char *fmt, ...) {
	va_list args;
	char *buf;

	if (!ginfo -> gui_info.status_window) {
		return;
	}

	va_start (args, fmt);

	buf = g_strdup_vprintf (fmt, args);

	va_end (args);

	StatusWindowWrite (ginfo -> gui_info.status_window, buf);

	g_free (buf);
}

#define HELP_FILE "grip.xml"

static void DoHelp (GtkWidget *widget, gpointer data) {
	char *section, *yelp_exe;

	section = (char *)data;

	yelp_exe = g_find_program_in_path ("yelp");
	if (g_path_is_absolute (yelp_exe)) {
		GError *error;
		gchar *argv[3], *cwd, *file;

		cwd = g_get_current_dir();
		//file = g_build_filename (cwd, HELP_FILE, NULL);
		file = g_build_filename (HELP_DIR, HELP_FILE, NULL);

		argv[0] = yelp_exe;

		if (section) {
			argv[1] = g_strdup_printf ("ghelp://%s?%s", file, section);
		} else {
			argv[1] = g_strdup_printf ("ghelp://%s", file);
		}

		argv[2] = NULL;

		g_free (cwd);
		g_free (file);


		error = NULL;

		if (!g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, NULL, &error)) {
			gchar *errmsg;

			errmsg = g_strdup_printf (_("Unable to run yelp: %s"), error -> message);
			show_error (NULL, errmsg);
			g_free (errmsg);
		}

		g_free (argv[1]);
		g_free (yelp_exe);
	} else {
		show_error (NULL, _("Cannot open help file: yelp was not found in PATH"));
	}
}

static void MakeHelpPage (GripInfo *ginfo) {
	GtkWidget *help_page;
	GtkWidget *button;
	GtkWidget *vbox;

	help_page = MakeNewPage (ginfo -> gui_info.notebook, _("Help"));

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	button = gtk_button_new_with_label (_("Table Of Contents"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Playing CDs"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "cdplayer");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Ripping CDs"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "ripping");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Configuring Grip"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "configure");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("FAQ"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "faq");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Getting More Help"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "morehelp");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Reporting Bugs"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (DoHelp), (gpointer) "bugs");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_container_add (GTK_CONTAINER (help_page), vbox);
	gtk_widget_show (vbox);
}

void MakeAboutPage (GripGUI *uinfo) {
	GtkWidget *aboutpage;
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *label;
	GtkWidget *logo;
	GtkWidget *ebox;
	GtkWidget *button;
	char versionbuf[20];

	aboutpage = MakeNewPage (uinfo -> notebook, _("About"));

	ebox = gtk_event_box_new();
	gtk_widget_set_style (ebox, uinfo -> style_wb);

	vbox = gtk_vbox_new (TRUE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

#ifndef GRIPCD
	logo = Loadxpm (GTK_WIDGET (uinfo -> app), grip_xpm);
#else
	logo = Loadxpm (GTK_WIDGET (uinfo -> app), gcd_xpm);
#endif

	gtk_box_pack_start (GTK_BOX (vbox), logo, FALSE, FALSE, 0);
	gtk_widget_show (logo);

	vbox2 = gtk_vbox_new (TRUE, 0);

	sprintf (versionbuf, _("Version %s"), VERSION);
	label = gtk_label_new (versionbuf);
	gtk_widget_set_style (label, uinfo -> style_wb);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	label = gtk_label_new ("Copyright 1998-2014, Mike Oliphant");
	gtk_widget_set_style (label, uinfo -> style_wb);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	label = gtk_label_new ("De-gnomization by Giorgio Moscardi");
	gtk_widget_set_style (label, uinfo -> style_wb);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

#if defined(__sun__)
	label = gtk_label_new ("Solaris Port, David Meleedy");
	gtk_widget_set_style (label, uinfo -> style_wb);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
#endif

	hbox = gtk_hbox_new (TRUE, 0);

	button = gtk_button_new_with_label ("http://sourceforge.net/projects/grip/");
	gtk_widget_set_style (button, uinfo -> style_dark_grey);
	gtk_widget_set_style (GTK_BIN (button) -> child,
	                      uinfo -> style_dark_grey);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (Homepage), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);


	gtk_container_add (GTK_CONTAINER (vbox), vbox2);
	gtk_widget_show (vbox2);

	gtk_container_add (GTK_CONTAINER (ebox), vbox);
	gtk_widget_show (vbox);

	gtk_container_add (GTK_CONTAINER (aboutpage), ebox);
	gtk_widget_show (ebox);
}

static void MakeStyles (GripGUI *uinfo) {
	GdkColor gdkblack;
	GdkColor gdkwhite;
	GdkColor *color_LCD;
	GdkColor *color_dark_grey;

	gdk_color_white (gdk_colormap_get_system (), &gdkwhite);
	gdk_color_black (gdk_colormap_get_system (), &gdkblack);

	color_LCD = MakeColor (33686, 38273, 29557);
	color_dark_grey = MakeColor (0x4444, 0x4444, 0x4444);

	uinfo -> style_wb = MakeStyle (&gdkwhite, &gdkblack, FALSE);
	uinfo -> style_LCD = MakeStyle (color_LCD, color_LCD, FALSE);
	uinfo -> style_dark_grey = MakeStyle (&gdkwhite, color_dark_grey, TRUE);
}

static void Homepage (void) {
	system ("xdg-open http://sourceforge.net/projects/grip/");
}

static void LoadImages (GripGUI *uinfo) {
	uinfo -> check_image = Loadxpm (uinfo -> app, check_xpm);
	uinfo -> eject_image = Loadxpm (uinfo -> app, eject_xpm);
	uinfo -> cdscan_image = Loadxpm (uinfo -> app, cdscan_xpm);
	uinfo -> ff_image = Loadxpm (uinfo -> app, ff_xpm);
	uinfo -> lowleft_image = Loadxpm (uinfo -> app, lowleft_xpm);
	uinfo -> lowright_image = Loadxpm (uinfo -> app, lowright_xpm);
	uinfo -> minmax_image = Loadxpm (uinfo -> app, minmax_xpm);
	uinfo -> nexttrk_image = Loadxpm (uinfo -> app, nexttrk_xpm);
	uinfo -> playpaus_image = Loadxpm (uinfo -> app, playpaus_xpm);
	uinfo -> prevtrk_image = Loadxpm (uinfo -> app, prevtrk_xpm);
	uinfo -> loop_image = Loadxpm (uinfo -> app, loop_xpm);
	uinfo -> noloop_image = Loadxpm (uinfo -> app, noloop_xpm);
	uinfo -> random_image = Loadxpm (uinfo -> app, random_xpm);
	uinfo -> playlist_image = Loadxpm (uinfo -> app, playlist_xpm);
	uinfo -> playnorm_image = Loadxpm (uinfo -> app, playnorm_xpm);
	uinfo -> quit_image = Loadxpm (uinfo -> app, quit_xpm);
	uinfo -> rew_image = Loadxpm (uinfo -> app, rew_xpm);
	uinfo -> stop_image = Loadxpm (uinfo -> app, stop_xpm);
	uinfo -> upleft_image = Loadxpm (uinfo -> app, upleft_xpm);
	uinfo -> upright_image = Loadxpm (uinfo -> app, upright_xpm);
	uinfo -> vol_image = Loadxpm (uinfo -> app, vol_xpm);
	uinfo -> discdbwht_image = Loadxpm (uinfo -> app, discdbwht_xpm);
	uinfo -> rotate_image = Loadxpm (uinfo -> app, rotate_xpm);
	uinfo -> edit_image = Loadxpm (uinfo -> app, edit_xpm);
	uinfo -> progtrack_image = Loadxpm (uinfo -> app, progtrack_xpm);
	uinfo -> mail_image = Loadxpm (uinfo -> app, mail_xpm);
	uinfo -> save_image = Loadxpm (uinfo -> app, save_xpm);

	uinfo -> empty_image = NewBlankPixmap (uinfo -> app);

	uinfo -> discdb_pix[0] = Loadxpm (uinfo -> app, discdb0_xpm);
	uinfo -> discdb_pix[1] = Loadxpm (uinfo -> app, discdb1_xpm);

	uinfo -> play_pix[0] = Loadxpm (uinfo -> app, playnorm_xpm);
	uinfo -> play_pix[1] = Loadxpm (uinfo -> app, random_xpm);
	uinfo -> play_pix[2] = Loadxpm (uinfo -> app, playlist_xpm);

#ifndef GRIPCD
	uinfo -> rip_pix[0] = Loadxpm (uinfo -> app, rip0_xpm);
	uinfo -> rip_pix[1] = Loadxpm (uinfo -> app, rip1_xpm);
	uinfo -> rip_pix[2] = Loadxpm (uinfo -> app, rip2_xpm);
	uinfo -> rip_pix[3] = Loadxpm (uinfo -> app, rip3_xpm);

	uinfo -> mp3_pix[0] = Loadxpm (uinfo -> app, enc0_xpm);
	uinfo -> mp3_pix[1] = Loadxpm (uinfo -> app, enc1_xpm);
	uinfo -> mp3_pix[2] = Loadxpm (uinfo -> app, enc2_xpm);
	uinfo -> mp3_pix[3] = Loadxpm (uinfo -> app, enc3_xpm);

	uinfo -> smile_pix[0] = Loadxpm (uinfo -> app, smile1_xpm);
	uinfo -> smile_pix[1] = Loadxpm (uinfo -> app, smile2_xpm);
	uinfo -> smile_pix[2] = Loadxpm (uinfo -> app, smile3_xpm);
	uinfo -> smile_pix[3] = Loadxpm (uinfo -> app, smile4_xpm);
	uinfo -> smile_pix[4] = Loadxpm (uinfo -> app, smile5_xpm);
	uinfo -> smile_pix[5] = Loadxpm (uinfo -> app, smile6_xpm);
	uinfo -> smile_pix[6] = Loadxpm (uinfo -> app, smile7_xpm);
	uinfo -> smile_pix[7] = Loadxpm (uinfo -> app, smile8_xpm);
#endif
}

void GripUpdate (GtkWidget *app) {
	GripInfo *ginfo;
	time_t secs;

	ginfo = (GripInfo *) gtk_object_get_user_data (GTK_OBJECT (app));

	if (ginfo -> ffwding) {
		FastFwd (ginfo);
	}

	if (ginfo -> rewinding) {
		Rewind (ginfo);
	}

	secs = time (NULL);

	/* Make sure we don't mod by zero */
	if (!ginfo -> poll_interval) {
		ginfo -> poll_interval = 1;
	}

	if (ginfo -> ripping) {
		UpdateRipProgress (ginfo);
	} else {
		if (ginfo -> poll_drive && ! (secs % ginfo -> poll_interval)) {
			if (!ginfo -> have_disc) {
				CheckNewDisc (ginfo, FALSE);
			}
		}

		UpdateDisplay (ginfo);
	}

	UpdateTray (ginfo);
}

void Busy (GripGUI *uinfo) {
	gdk_window_set_cursor (uinfo -> app -> window, uinfo -> wait_cursor);

	UpdateGTK();
}

void UnBusy (GripGUI *uinfo) {
	gdk_window_set_cursor (uinfo -> app -> window, NULL);

	UpdateGTK();
}

static void set_initial_config (GripInfo *ginfo) {
	GripGUI *uinfo = & (ginfo -> gui_info);
//	gchar *filename;
//	char renamefile[256];
	char *proxy_env, *tok;
//	char outputdir[256];
//	int confret;
//	CFGEntry cfg_entries[] = {
//		CFG_ENTRIES
//		{"outputdir", CFG_ENTRY_STRING, 256, outputdir},
//		{"", CFG_ENTRY_LAST, 0, NULL}
//	};

//	outputdir[0] = '\0';

	uinfo -> minimized = FALSE;
	uinfo -> volvis = FALSE;
	uinfo -> track_prog_visible = FALSE;
	uinfo -> track_edit_visible = FALSE;

	uinfo -> wait_cursor = gdk_cursor_new (GDK_WATCH);

	uinfo -> tray_icon = NULL;

	uinfo -> id3_genre_item_list = NULL;

	*ginfo -> version = '\0';

	// Load CD device settings
	gchar *tmp = g_settings_get_string (ginfo -> settings, "cd-device");
    strncpy (ginfo -> cd_device, tmp, sizeof (ginfo -> cd_device));
    g_free (tmp);
	tmp = g_settings_get_string (ginfo -> settings_cdparanoia, "force-scsi");
    strncpy (ginfo -> force_scsi, tmp, sizeof (ginfo -> force_scsi));
    g_free (tmp);

	ginfo -> local_mode = FALSE;
	ginfo -> have_disc = FALSE;
	ginfo -> tray_open = FALSE;
//	ginfo -> faulty_eject = FALSE;
	ginfo -> looking_up = FALSE;
	ginfo -> play_mode = PM_NORMAL;
	ginfo -> playloop = TRUE;
//	ginfo -> automatic_reshuffle = TRUE;
	ginfo -> ask_submit = FALSE;
	ginfo -> is_new_disc = FALSE;
	ginfo -> first_time = TRUE;
//	ginfo -> automatic_discdb = TRUE;
	ginfo -> auto_eject_countdown = 0;
	ginfo -> current_discid = 0;
	ginfo -> volume = 255;
//#if defined(__FreeBSD__) || defined(__NetBSD__)
//	ginfo -> poll_drive = FALSE;
//	ginfo -> poll_interval = 15;
//#else
//	ginfo -> poll_drive = TRUE;
//	ginfo -> poll_interval = 1;
//#endif

	ginfo -> changer_slots = 0;
	ginfo -> current_disc = 0;

	ginfo -> proxy_server.name[0] = '\0';
	ginfo -> proxy_server.port = 8000;
	ginfo -> use_proxy = FALSE;
	ginfo -> use_proxy_env = FALSE;

//	strcpy (ginfo -> dbserver.name, "freedb.freedb.org");
//	strcpy (ginfo -> dbserver.cgi_prog, "~cddb/cddb.cgi");
//	ginfo -> dbserver.port = 80;
//	ginfo -> dbserver.use_proxy = 0;
//	ginfo -> dbserver.proxy = & (ginfo -> proxy_server);

//	strcpy (ginfo -> discdb_submit_email, "freedb-submit@freedb.org");
//	ginfo -> db_use_freedb = TRUE;
	*ginfo -> user_email = '\0';

	strcpy (ginfo -> discdb_encoding, "UTF-8");
	strcpy (ginfo -> id3_encoding, "UTF-8");
//	strcpy (ginfo -> id3v2_encoding, "UTF-8");

	ginfo -> update_required = FALSE;
//	ginfo -> automatic_discdb = TRUE;
//	ginfo -> play_first = TRUE;
//	ginfo -> play_on_insert = FALSE;
//	ginfo -> stop_first = FALSE;
//	ginfo -> no_interrupt = FALSE;
	ginfo -> playing = FALSE;
	ginfo -> stopped = FALSE;
	ginfo -> ffwding = FALSE;
	ginfo -> rewinding = FALSE;

	strcpy (ginfo -> title_split_chars, "/");

	ginfo -> curr_pipe_fd = -1;

	ginfo -> ripping = FALSE;
//	ginfo -> ripping_a_disc = FALSE;
//	ginfo -> encoding = FALSE;
	ginfo -> rip_partial = FALSE;
	ginfo -> stop_rip = FALSE;
//	ginfo -> stop_encode = FALSE;
//	ginfo -> rip_finished = 0;
//	ginfo -> num_wavs = 0;
//	ginfo -> doencode = FALSE;
//	ginfo -> encode_list = NULL;
//	ginfo -> pending_list = NULL;
	ginfo -> do_redirect = TRUE;

	ginfo -> rip_thread = NULL;
	ginfo -> encoder_data = NULL;

//	ginfo -> stop_thread_rip_now = FALSE;
//	ginfo -> disable_paranoia = FALSE;
//	ginfo -> disable_extra_paranoia = FALSE;
//	ginfo -> disable_scratch_detect = FALSE;
//	ginfo -> disable_scratch_repair = FALSE;
//	ginfo -> calc_gain = FALSE;
	ginfo -> in_rip_thread = FALSE;

//	strcpy (ginfo -> ripfileformat, "~/Music/%A/%d/%n");

//	ginfo -> max_wavs = 99;
//	ginfo -> auto_rip = FALSE;
//	ginfo -> beep_after_rip = TRUE;
//	ginfo -> eject_after_rip = TRUE;
//	ginfo -> eject_delay = 0;
//	ginfo -> delay_before_rip = FALSE;
//	ginfo -> stop_between_tracks = FALSE;
//	*ginfo -> wav_filter_cmd = '\0';
//	*ginfo -> disc_filter_cmd = '\0';
//	ginfo -> selected_encoder = 1;
//  strcpy(ginfo -> mp3cmdline,"-h -b %b %w %m");
//  FindExeInPath("lame", ginfo -> mp3exename, sizeof(ginfo -> mp3exename));
//  strcpy(ginfo -> mp3fileformat,"~/mp3/%A/%d/%n.%x");
//  strcpy(ginfo -> mp3extension,"mp3");
//	ginfo -> mp3nice = 0;
//	*ginfo -> mp3_filter_cmd = '\0';
//	ginfo -> delete_wavs = TRUE;
//	ginfo -> add_m3u = TRUE;
//	ginfo -> rel_m3u = TRUE;
//	strcpy (ginfo -> m3ufileformat, "~/Music/%A-%d.m3u");
//	ginfo -> kbits_per_sec = 128;
//	ginfo -> edit_num_cpu = 1;
//	ginfo -> doid3 = TRUE;
//	ginfo -> doid3 = FALSE;
//	ginfo -> tag_mp3_only = TRUE;
//	strcpy (ginfo -> id3_comment, _("Ripped with Regrip"));
	*ginfo -> cdupdate = '\0';
	ginfo -> sprefs.no_lower_case = FALSE;
	ginfo -> sprefs.allow_high_bits = FALSE;
	ginfo -> sprefs.escape = FALSE;
	ginfo -> sprefs.no_underscore = FALSE;
	*ginfo -> sprefs.allow_these_chars = '\0';
	ginfo -> show_tray_icon = TRUE;

#if 0
	filename = g_build_filename (g_get_home_dir(), ginfo -> config_filename, NULL);

	confret = LoadConfig (filename, "GRIP", 2, 2, cfg_entries);

	if (confret < 0) {
		/* Check if the config is out of date */
		if (confret == -2) {
			show_warning (ginfo -> gui_info.app,
			              _("Your config file is out of date -- "
			                 "resetting to defaults.\n"
			                 "You will need to re-configure Grip.\n"
			                 "Your old config file has been saved with -old appended."));

			sprintf (renamefile, "%s-old", filename);

			rename (filename, renamefile);
		}

		DoSaveConfig (ginfo);
	}

	LoadEncoderConfig (ginfo, ginfo -> selected_encoder);

#ifndef GRIPCD
	/* Phase out 'outputdir' variable */

//	if (*outputdir) {
//		strcpy (filename, outputdir);
//		MakePath (filename);
//    strcat(filename,ginfo -> mp3fileformat);
//    strcpy(ginfo -> mp3fileformat,filename);

//		strcpy (filename, outputdir);
//    MakePath(filename);
//    strcat(filename,ginfo -> ripfileformat);
//    strcpy(ginfo -> ripfileformat,filename);

//		*outputdir = '\0';
//	}

#endif

	g_free (filename);
#endif

	if (!*ginfo -> user_email) {
		char *host;
		char *user;

		host = getenv ("HOST");
		if (!host) {
			host = getenv ("HOSTNAME");
		}
		if (!host) {
			host = "localhost";
		}

		user = getenv ("USER");
		if (!user) {
			user = getenv ("USERNAME");
		}
		if (!user) {
			user = "user";
		}

		g_snprintf (ginfo -> user_email, 256, "%s@%s", user, host);
	}

	if (ginfo -> use_proxy_env) {  /* Get proxy info from "http_proxy" */
		proxy_env = getenv ("http_proxy");

		if (proxy_env) {
			/* Skip the "http://" if it's present */
			if (!strncasecmp (proxy_env, "http://", 7)) {
				proxy_env += 7;
			}

			tok = strtok (proxy_env, ":");
			if (tok) {
				strncpy (ginfo -> proxy_server.name, tok, 256);
			}

			tok = strtok (NULL, "/");
			if (tok) {
				ginfo -> proxy_server.port = atoi (tok);
			}

			g_debug (_("server is %s, port %d"), ginfo -> proxy_server.name,
			         ginfo -> proxy_server.port);
		}
	}
}

/* Shut down stuff (generally before an exec) */
void CloseStuff (void *user_data) {
	GripInfo *ginfo;
	int fd;

	ginfo = (GripInfo *) user_data;

	close (ConnectionNumber (GDK_DISPLAY()));
	close (ginfo -> disc.cd_desc);

	fd = open ("/dev/null", O_RDWR);
	dup2 (fd, 0);

	if (ginfo -> do_redirect) {
		if (ginfo -> curr_pipe_fd > 0) {
			dup2 (ginfo -> curr_pipe_fd, 1);
			dup2 (ginfo -> curr_pipe_fd, 2);

			ginfo -> curr_pipe_fd = -1;
		} else {
			dup2 (fd, 1);
			dup2 (fd, 2);
		}
	}

	/* Close any other filehandles that might be around */
	for (fd = 3; fd < NOFILE; fd++) {
		close (fd);
	}
}
