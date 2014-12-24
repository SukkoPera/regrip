/* gripcfg.c
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

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "grip.h"
#include "gripcfg.h"
#include "dialog.h"
#include "parsecfg.h"
#include "common.h"
#include "encoder.h"


static void UseProxyChanged (GtkWidget *widget, gpointer data);
static void EncoderSelected (GtkComboBox *widget, gpointer data);

// From settings (uint) to object property (text)
static gboolean gsettings_map_int_to_string (GValue *value, GVariant *variant, gpointer user_data) {
    guint32 i;
    char p[8];

    i = g_variant_get_uint32 (variant);
    snprintf (p, sizeof (p), "%u", i);
    g_value_set_string (value, p);
    return TRUE;
}

// From object property (text) to settings (uint)
static GVariant *gsettings_map_string_to_int (const GValue *value, const GVariantType *expected_type, gpointer user_data) {
    const char *s;
    char *p;
    int i;
    GVariant *variant;

//    g_debug ("GValue holds a %s", g_type_name (G_VALUE_TYPE (value)));

    s = g_value_get_string (value);
    i = strtol (s, &p, 10);
    if (*s != '\0' && *p == '\0')
        variant = g_variant_new_uint32 (i);
    else
        variant = NULL;

    return variant;
}


static void UseProxyChanged (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *)data;

	ginfo -> dbserver2.use_proxy = ginfo -> dbserver.use_proxy = ginfo -> use_proxy;
}

void MakeConfigPage (GripInfo *ginfo) {
	GripGUI *uinfo;
	GtkWidget *vbox, *vbox2, *dbvbox;
	GtkWidget *entry;
	GtkWidget *realentry;
	GtkWidget *label;
	GtkWidget *check;
	GtkWidget *config_notebook;
	GtkWidget *configpage;
	GtkWidget *button;
#ifndef GRIPCD
	GtkWidget *hbox;
	GtkWidget *menu;
#endif

	uinfo = & (ginfo -> gui_info);

	configpage = MakeNewPage (uinfo -> notebook, _("Config"));

	vbox2 = gtk_vbox_new (FALSE, 0);
	config_notebook = gtk_notebook_new ();

    /*************************************************************************/
	/* CD PAGE                                                               */
	/*************************************************************************/

	vbox = gtk_vbox_new (FALSE, 2);

	hbox = MakeStrEntry (&entry, ginfo -> cd_device, _("CD-Rom device"), 255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	g_settings_bind (ginfo -> settings, "cd-device", entry, "text", G_SETTINGS_BIND_DEFAULT);


	check = MakeCheckButton (NULL, &ginfo -> no_interrupt,
	                         _("Don't interrupt playback on exit/startup"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "no-interrupt", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> stop_first, _("Rewind when stopped"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "stop-first", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> play_first,
	                         _("Startup with first track if not playing"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "play-first", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> play_on_insert,
	                         _("Auto-play on disc insert"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "play-on-insert", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> automatic_reshuffle,
	                         _("Reshuffle before each playback"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "automatic-reshuffle", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> faulty_eject,
	                         _("Work around faulty eject"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "faulty-eject", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> poll_drive,
	                         _("Poll disc drive for new disc"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdplay, "poll-drive", check, "active", G_SETTINGS_BIND_DEFAULT);

	hbox = MakeNumEntry (&entry, &ginfo -> poll_interval,
	                      _("Poll interval (seconds)"),
	                      3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	g_settings_bind_with_mapping (ginfo -> settings_cdplay, "poll-interval", entry, "text", G_SETTINGS_BIND_DEFAULT, gsettings_map_int_to_string, gsettings_map_string_to_int, NULL, NULL);

	label = gtk_label_new (_("CD"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);


	/*************************************************************************/
	/* RIP PAGE                                                              */
	/*************************************************************************/

#ifndef GRIPCD
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);


	hbox = gtk_hbox_new (FALSE, 3);

	check = MakeCheckButton (NULL, &ginfo -> auto_rip, _("Auto-rip on insert"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_rip, "auto-rip", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> beep_after_rip, _("Beep after rip"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_rip, "beep-after-rip", check, "active", G_SETTINGS_BIND_DEFAULT);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);


	hbox = gtk_hbox_new (FALSE, 3);

	check = MakeCheckButton (NULL, &ginfo -> eject_after_rip,
	                         _("Auto-eject after rip"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_rip, "eject-after-rip", check, "active", G_SETTINGS_BIND_DEFAULT);

	// Use menu here since I don't want to create another variable just for this :)
	menu = MakeNumEntry (&entry, &ginfo -> eject_delay, _("Auto-eject delay"), 3);
	gtk_box_pack_start (GTK_BOX (hbox), menu, TRUE, TRUE, 0);
	gtk_widget_show (menu);
	g_settings_bind_with_mapping (ginfo -> settings_rip, "eject-delay", entry, "text", G_SETTINGS_BIND_DEFAULT, gsettings_map_int_to_string, gsettings_map_string_to_int, NULL, NULL);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	check = MakeCheckButton (NULL, &ginfo -> delay_before_rip,
	                         _("Delay before ripping"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_rip, "delay-before-rip", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> stop_between_tracks,
	                         _("Stop CD-Rom drive between tracks"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_rip, "stop-between-tracks", check, "active", G_SETTINGS_BIND_DEFAULT);


		hbox = gtk_hbox_new (FALSE, 2);

	check = MakeCheckButton (NULL, &ginfo -> disable_paranoia,
	                         _("Disable paranoia"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdparanoia, "disable-paranoia", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> disable_extra_paranoia,
	                         _("Disable extra paranoia"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdparanoia, "disable-extra-paranoia", check, "active", G_SETTINGS_BIND_DEFAULT);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);


	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Disable scratch"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	check = MakeCheckButton (NULL, &ginfo -> disable_scratch_detect, _("detection"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdparanoia, "disable-scratch-detect", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> disable_scratch_repair, _("repair"));
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cdparanoia, "disable-scratch-repair", check, "active", G_SETTINGS_BIND_DEFAULT);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = MakeStrEntry (&entry, ginfo -> force_scsi, _("Generic SCSI device"),
	                      255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	g_settings_bind (ginfo -> settings_cdparanoia, "force-scsi", entry, "text", G_SETTINGS_BIND_DEFAULT);


	hbox = MakeStrEntry (&entry, ginfo -> wav_filter_cmd, _("WAV filter command"),
	                      255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	g_settings_bind (ginfo -> settings_rip, "wav-filter-cmd", entry, "text", G_SETTINGS_BIND_DEFAULT);

	hbox = MakeStrEntry (&entry, ginfo -> disc_filter_cmd, _("Disc filter command"),
	                      255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	g_settings_bind (ginfo -> settings_rip, "disc-filter-cmd", entry, "text", G_SETTINGS_BIND_DEFAULT);

	label = gtk_label_new (_("Rip"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);


    /*************************************************************************/
	/* ENCODE PAGE                                                           */
	/*************************************************************************/

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Output File Format:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	/* *** Encoders menu START *** */
    gchar *selected_encoder, *selected_format;
    g_settings_get (ginfo -> settings, "selected-encoder", "s", &selected_encoder);
    g_settings_get (ginfo -> settings, "selected-format", "s", &selected_format);
//    g_debug ("Selected encoder/format: %s/%s", selected_encoder, selected_format);

	GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);    // Name, supported_format, supported_encoder
	GtkTreeIter iter, iter_to_select;
	gboolean selected_found = FALSE;
	supported_encoder **enc = supported_encoders;
	while (*enc) {
        g_debug ("Registering encoder: %s", (*enc) -> name);
        supported_format *fmt = (*enc) -> supported_formats;
        while (fmt -> name) {
            g_debug ("Registering format: %s", fmt -> name);
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, fmt -> name, 1, fmt, 2, *enc, -1);

            if (strcmp ((*enc) -> name, selected_encoder) == 0 && strcmp (fmt -> name, selected_format) == 0) {
//                g_debug ("Found selected format");
                iter_to_select = iter;
                selected_found = TRUE;
            }

            ++fmt;
        }
        ++enc;
	}

	menu = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));

	/* liststore is now owned by combo, so the initial reference can
     * be dropped */
	g_object_unref (G_OBJECT (store));


    /* Create cell renderer. */
    GtkCellRenderer *cell = gtk_cell_renderer_text_new ();

    /* Pack it to the combo box. */
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (menu), cell, TRUE);

    /* Connect renderer to data source */
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (menu), cell, "text", 0, NULL);

	g_signal_connect (GTK_OBJECT (menu), "changed",
	                    G_CALLBACK (EncoderSelected), (gpointer) ginfo);

    // Make default encoder selected. This MUST be called after connecting the signal handler
    if (selected_found)
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (menu), &iter_to_select);
    else        // Select first available encoder
        gtk_combo_box_set_active (GTK_COMBO_BOX (menu), 1);

	gtk_widget_show (menu);
	gtk_box_pack_start (GTK_BOX (hbox), menu, TRUE, TRUE, 0);
	/* *** Encoders menu END *** */

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	check = MakeCheckButton (NULL, &ginfo -> calc_gain,
	                         _("Calculate gain adjustment"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
//	g_settings_bind (ginfo -> settings_cdparanoia, "disable-extra-paranoia", check, "active", G_SETTINGS_BIND_DEFAULT);

	check = MakeCheckButton (NULL, &ginfo -> add_m3u, _("Create .m3u files"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> rel_m3u,
	                         _("Use relative paths in .m3u files"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	entry = MakeStrEntry (NULL, ginfo -> m3ufileformat, _("M3U file format"), 255,
	                      TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeNumEntry (NULL, &ginfo -> kbits_per_sec,
	                      _("Encoding bitrate (kbits/sec)"), 3);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeNumEntry (NULL, &ginfo -> edit_num_cpu, _("Number of CPUs to use"),
	                      3);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeNumEntry (NULL, &ginfo -> mp3nice, _("Encode 'nice' value"), 3);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> mp3_filter_cmd, _("Encode filter command"),
	                      255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	label = gtk_label_new (_("Encode"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);


    /*************************************************************************/
	/* ID3 PAGE                                                              */
	/*************************************************************************/

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	check = MakeCheckButton (NULL, &ginfo -> doid3,
	                         _("Add ID3 tags to encoded files"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

#ifdef HAVE_ID3V2
	check = MakeCheckButton (NULL, &ginfo -> doid3v2,
	                         _("Add ID3v2 tags to encoded files"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
#endif

	check = MakeCheckButton (NULL, &ginfo -> tag_mp3_only,
	                         _("Only tag files ending in '.mp3'"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	entry = MakeStrEntry (NULL, ginfo -> id3_comment, _("ID3 comment field"), 29,
	                      TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> id3_encoding,
	                      _("ID3v1 Character set encoding"), 16, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

#ifdef HAVE_ID3V2
	entry = MakeStrEntry (NULL, ginfo -> id3v2_encoding,
	                      _("ID3v2 Character set encoding"), 16, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);
#endif

	label = gtk_label_new (_("ID3"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);
#endif


    /*************************************************************************/
	/* DISCDB PAGE                                                           */
	/*************************************************************************/

	dbvbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (dbvbox), 3);

	entry = MakeStrEntry (NULL, ginfo -> dbserver.name, _("DB server"), 255, TRUE);
	gtk_box_pack_start (GTK_BOX (dbvbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> discdb_submit_email,
	                      _("DB Submit email"), 255, TRUE);
	gtk_box_pack_start (GTK_BOX (dbvbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> discdb_encoding,
	                      _("DB Character set encoding"), 16, TRUE);
	gtk_box_pack_start (GTK_BOX (dbvbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	check = MakeCheckButton (NULL, &ginfo -> db_use_freedb,
	                         _("Use freedb extensions"));
	gtk_box_pack_start (GTK_BOX (dbvbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> automatic_discdb,
	                         _("Perform disc lookups automatically"));
	gtk_box_pack_start (GTK_BOX (dbvbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);
	g_settings_bind (ginfo -> settings_cddb, "automatic-discdb", check, "active", G_SETTINGS_BIND_DEFAULT);

	label = gtk_label_new (_("DiscDB"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), dbvbox, label);
	gtk_widget_show (dbvbox);


    /*************************************************************************/
	/* PROXY PAGE                                                            */
	/*************************************************************************/

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	check = MakeCheckButton (&button, &ginfo -> use_proxy, _("Use proxy server"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (UseProxyChanged), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> use_proxy_env,
	                         _("Get server from 'http_proxy' env. var"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	entry = MakeStrEntry (NULL, ginfo -> proxy_server.name, _("Proxy server"), 255,
	                      TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeNumEntry (NULL, & (ginfo -> proxy_server.port), _("Proxy port"), 5);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> proxy_server.username, _("Proxy username"),
	                      80, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (&realentry, ginfo -> proxy_server.pswd,
	                      _("Proxy password"), 80, TRUE);
	gtk_entry_set_visibility (GTK_ENTRY (realentry), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	label = gtk_label_new (_("Proxy"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);


    /*************************************************************************/
	/* MISC PAGE                                                              */
	/*************************************************************************/

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	entry = MakeStrEntry (NULL, ginfo -> user_email, _("Email address"), 255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	entry = MakeStrEntry (NULL, ginfo -> cdupdate, _("CD update program"), 255,
	                      TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	check = MakeCheckButton (NULL, &ginfo -> sprefs.no_lower_case,
	                         _("Do not lowercase filenames"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> sprefs.allow_high_bits,
	                         _("Allow high bits in filenames"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> sprefs.escape,
	                         _("Replace incompatible characters by hexadecimal numbers"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	check = MakeCheckButton (NULL, &ginfo -> sprefs.no_underscore,
	                         _("Do not change spaces to underscores"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	entry = MakeStrEntry (NULL, ginfo -> sprefs.allow_these_chars,
	                      _("Characters to not strip\nin filenames"), 255, TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	check = MakeCheckButton (NULL, &ginfo -> show_tray_icon,
	                         _("Show tray icon"));
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	label = gtk_label_new (_("Misc"));
	gtk_notebook_append_page (GTK_NOTEBOOK (config_notebook), vbox, label);
	gtk_widget_show (vbox);



	gtk_box_pack_start (GTK_BOX (vbox2), config_notebook, FALSE, FALSE, 0);
	gtk_widget_show (config_notebook);

	gtk_container_add (GTK_CONTAINER (configpage), vbox2);
	gtk_widget_show (vbox2);
}


static void EncoderSelected (GtkComboBox *widget, gpointer data) {
	GripInfo *ginfo;
//	GripGUI *uinfo;

	ginfo = (GripInfo *) data;
//	uinfo = &(ginfo -> gui_info);

	GtkTreeIter iter;
    GtkTreeModel *model;

    /* Obtain currently selected item form combo box.
     * If nothing is selected, do nothing. */
    if (gtk_combo_box_get_active_iter(widget, &iter)) {
        /* Obtain data model from combo box. */
        model = gtk_combo_box_get_model (widget);

        /* Obtain encoder from model. */
        gchar *format_name;
        gtk_tree_model_get (model, &iter, 0, &format_name, 1, &(ginfo -> format), 2, &(ginfo -> encoder), -1);
        g_debug ("Selected format: %s", format_name);

        // Save selection
        g_settings_set (ginfo -> settings, "selected-encoder", "s", ginfo -> encoder -> name);
        g_settings_set (ginfo -> settings, "selected-format", "s", format_name);
    }
}
