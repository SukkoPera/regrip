/* discedit.c
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

#include <glib.h>
#include <glib/gi18n.h>
#include "grip.h"
#include "cdplay.h"
#include "dialog.h"
#include "uihelper.h"
#include "discedit.h"


#define GENRES_NUM 192

/**
 * list of canonical ID3v1 genre names in the order that they
 * are listed in the standard.
 *
 * Taken from TagLib git repository on 2014/12/29.
 */
static const char *genres[GENRES_NUM] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "Alternative Rock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychedelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk/Rock",
    "National Folk",
    "Swing",
    "Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "A Cappella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta Rap",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary Christian",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "Jpop",
    "Synthpop",
    "Abstract",
    "Art Rock",
    "Baroque",
    "Bhangra",
    "Big Beat",
    "Breakbeat",
    "Chillout",
    "Downtempo",
    "Dub",
    "EBM",
    "Eclectic",
    "Electro",
    "Electroclash",
    "Emo",
    "Experimental",
    "Garage",
    "Global",
    "IDM",
    "Illbient",
    "Industro-Goth",
    "Jam Band",
    "Krautrock",
    "Leftfield",
    "Lounge",
    "Math Rock",
    "New Romantic",
    "Nu-Breakz",
    "Post-Punk",
    "Post-Rock",
    "Psytrance",
    "Shoegaze",
    "Space Rock",
    "Trop Rock",
    "World Music",
    "Neoclassical",
    "Audiobook",
    "Audio Theatre",
    "Neue Deutsche Welle",
    "Podcast",
    "Indie Rock",
    "G-Funk",
    "Dubstep",
    "Garage Rock",
    "Psybient"
};

#if 0
/* This array maps CDDB_ genre numbers to closest id3 genre */
int cddb_2_id3[] = {
	12,         /* CDDB_UNKNOWN */
	0,          /* CDDB_BLUES */
	32,         /* CDDB_CLASSICAL */
	2,          /* CDDB_COUNTRY */
	12,         /* CDDB_DATA */
	80,         /* CDDB_FOLK */
	8,          /* CDDB_JAZZ */
	12,         /* CDDB_MISC */
	10,         /* CDDB_NEWAGE */
	16,         /* CDDB_REGGAE */
	17,         /* CDDB_ROCK */
	24,         /* CDDB_SOUNDTRACK */
};
#endif

static void save_disc_info (GtkWidget *widget, gpointer data);
static void on_title_edit_changed (GtkWidget *widget, gpointer data);
static void on_artist_edit_changed (GtkWidget *widget, gpointer data);
static void on_year_edit_changed (GtkWidget *widget, gpointer data);
static void edit_next_track (GtkWidget *widget, gpointer data);
static void on_genre_changed (GtkWidget *widget, gpointer data);
static void separate_fields (char *buf, char *field1, char *field2, char *sep);
static void split_title_artist (GtkWidget *widget, gpointer data);
static void on_submit_entry (GtkWidget *widget, gpointer data);
static void get_discDBGenre (GripInfo *ginfo);


GtkWidget *MakeEditBox (GripInfo *ginfo) {
	GripGUI *uinfo;
	GtkWidget *vbox, *hbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *check;
	GtkWidget *entry;
	GtkObject *adj;
	int i, len;
	int dub_size;
	PangoLayout *layout;

	uinfo = &(ginfo -> gui_info);

	frame = gtk_frame_new (NULL);

	vbox = gtk_vbox_new (FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Disc title"));

	/* This should be the longest string in the track edit section */
	layout = gtk_widget_create_pango_layout (GTK_WIDGET (label),
	         _("Track name"));


	pango_layout_get_size (layout, &len, NULL);

	len /= PANGO_SCALE;

	g_object_unref (layout);

	layout = gtk_widget_create_pango_layout (GTK_WIDGET (label),
	         _("W"));

	pango_layout_get_size (layout, &dub_size, NULL);

	dub_size /= PANGO_SCALE;

	g_object_unref (layout);


	gtk_widget_set_usize (label, len, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	uinfo -> title_edit_entry = gtk_entry_new_with_max_length (MAX_STRING);
	gtk_signal_connect (GTK_OBJECT (uinfo -> title_edit_entry), "changed",
	                    GTK_SIGNAL_FUNC (on_title_edit_changed), (gpointer)ginfo);
	gtk_entry_set_position (GTK_ENTRY (uinfo -> title_edit_entry), 0);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> title_edit_entry, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> title_edit_entry);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Disc artist"));
	gtk_widget_set_usize (label, len, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	uinfo -> artist_edit_entry = gtk_entry_new_with_max_length (MAX_STRING);
	gtk_signal_connect (GTK_OBJECT (uinfo -> artist_edit_entry), "changed",
	                    GTK_SIGNAL_FUNC (on_artist_edit_changed), (gpointer)ginfo);
	gtk_entry_set_position (GTK_ENTRY (uinfo -> artist_edit_entry), 0);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> artist_edit_entry, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> artist_edit_entry);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Genre:"));
	gtk_widget_set_usize (label, len, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	/* For the genre list we use a combo with entry, populated with
	 * the standard ID3v1 genres, but since TagLib treats genres as
	 * a string, we accept everything. */
	uinfo -> genre_combo = gtk_combo_box_text_new_with_entry ();
    for (i = 0; i < GENRES_NUM; ++i) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (uinfo -> genre_combo), genres[i]);
    }

    // Enable autocompletion
    GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (uinfo -> genre_combo));
    GtkEntry *boxEntry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (uinfo -> genre_combo)));
    g_assert (boxEntry);
    GtkEntryCompletion *comp = gtk_entry_completion_new ();
    gtk_entry_completion_set_model (comp, model);
    gtk_entry_completion_set_text_column (comp, 0);
    gtk_entry_completion_set_minimum_key_length (comp, 1);
    gtk_entry_set_completion (boxEntry, comp);

    gtk_signal_connect (GTK_OBJECT (uinfo -> genre_combo), "changed", G_CALLBACK (on_genre_changed), ginfo);

	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> genre_combo, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> genre_combo);

	set_genre (ginfo, ginfo -> ddata.data_genre);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Disc year"));
	gtk_widget_set_usize (label, len, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	adj = gtk_adjustment_new (0, 0, 9999, 1.0, 5.0, 0);

	uinfo -> year_spin_button = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0.5, 0);
	gtk_signal_connect (GTK_OBJECT (uinfo -> year_spin_button), "value_changed",
	                    GTK_SIGNAL_FUNC (on_year_edit_changed), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> year_spin_button, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> year_spin_button);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Track name"));
	gtk_widget_set_usize (label, len, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	uinfo -> track_edit_entry = gtk_entry_new_with_max_length (MAX_STRING);
	gtk_signal_connect (GTK_OBJECT (uinfo -> track_edit_entry), "changed",
	                    GTK_SIGNAL_FUNC (on_track_edit_changed), (gpointer)ginfo);
	gtk_signal_connect (GTK_OBJECT (uinfo -> track_edit_entry), "activate",
	                    GTK_SIGNAL_FUNC (edit_next_track), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> track_edit_entry, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> track_edit_entry);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	uinfo -> multi_artist_box = gtk_vbox_new (FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Track artist"));
	gtk_widget_set_usize (label, len, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	uinfo -> track_artist_edit_entry = gtk_entry_new_with_max_length (MAX_STRING);
	gtk_signal_connect (GTK_OBJECT (uinfo -> track_artist_edit_entry), "changed",
	                    GTK_SIGNAL_FUNC (on_track_edit_changed), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> track_artist_edit_entry,
	                    TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> track_artist_edit_entry);

	gtk_box_pack_start (GTK_BOX (uinfo -> multi_artist_box), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("Split:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	button = gtk_button_new_with_label (_("Title/Artist"));
	gtk_object_set_user_data (GTK_OBJECT (button), (gpointer)0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (split_title_artist), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Artist/Title"));
	gtk_object_set_user_data (GTK_OBJECT (button), (gpointer)1);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (split_title_artist), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	entry = MakeStrEntry (&uinfo -> split_chars_entry, ginfo -> title_split_chars,
	                      _("Split chars"), 5, TRUE);

	gtk_widget_set_usize (uinfo -> split_chars_entry,
	                      5 * dub_size, 0);



	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	gtk_widget_show (entry);

	gtk_box_pack_start (GTK_BOX (uinfo -> multi_artist_box), hbox, FALSE, FALSE, 2);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (vbox), uinfo -> multi_artist_box, FALSE, FALSE, 0);

	if (ginfo -> ddata.data_multi_artist) {
		gtk_widget_show (uinfo -> multi_artist_box);
	}

	hbox = gtk_hbox_new (FALSE, 0);

	check = MakeCheckButton (&uinfo -> multi_artist_button,
	                         & (ginfo -> ddata.data_multi_artist),
	                         _("Multi-artist"));
	gtk_signal_connect (GTK_OBJECT (uinfo -> multi_artist_button), "clicked",
	                    GTK_SIGNAL_FUNC (update_multi_artist), (gpointer)ginfo);
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_widget_show (check);

	button = image_button (GTK_WIDGET (uinfo -> app), uinfo -> save_image);
	gtk_widget_set_style (button, uinfo -> style_dark_grey);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (save_disc_info), (gpointer)ginfo);
	gtk_tooltips_set_tip (make_tooltip(), button,
	                      _("Save disc info"), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = image_button (GTK_WIDGET (uinfo -> app), uinfo -> mail_image);
	gtk_widget_set_style (button, uinfo -> style_dark_grey);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	                    GTK_SIGNAL_FUNC (on_submit_entry), (gpointer)ginfo);
	gtk_tooltips_set_tip (make_tooltip(), button,
	                      _("Submit disc info"), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);

	return frame;
}

void update_multi_artist (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
	GripGUI *uinfo;

	ginfo = (GripInfo *) data;
	uinfo = &(ginfo -> gui_info);

	if (!ginfo -> ddata.data_multi_artist) {
		gtk_widget_hide (uinfo -> multi_artist_box);
		update_gtk ();
	} else {
		gtk_widget_show (uinfo -> multi_artist_box);
	}
}

void toggle_track_edit (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
	GripGUI *uinfo;

	ginfo = (GripInfo *) data;
	uinfo = &(ginfo -> gui_info);

	if (uinfo -> track_edit_visible) {
		gtk_window_resize (GTK_WINDOW (uinfo -> app),
		                   uinfo -> win_width,
		                   uinfo -> win_height);

		gtk_widget_hide (uinfo -> track_edit_box);
		update_gtk();
	} else {
		if (uinfo -> minimized) {
			min_max (NULL, (gpointer) ginfo);
		}

		gtk_widget_show (uinfo -> track_edit_box);

		gtk_window_resize (GTK_WINDOW (uinfo -> app),
		                   uinfo -> win_width,
		                   uinfo -> win_height_edit);
	}

	uinfo -> track_edit_visible = !uinfo -> track_edit_visible;
}

void set_title (GripInfo *ginfo, char *title) {
	g_signal_handlers_block_by_func (G_OBJECT (ginfo -> gui_info.title_edit_entry),
	                                 on_title_edit_changed, (gpointer) ginfo);

	gtk_entry_set_text (GTK_ENTRY (ginfo -> gui_info.title_edit_entry), title);
	gtk_entry_set_position (GTK_ENTRY (ginfo -> gui_info.title_edit_entry), 0);

	strcpy (ginfo -> ddata.data_title, title);
	gtk_label_set (GTK_LABEL (ginfo -> gui_info.disc_name_label), title);

	g_signal_handlers_unblock_by_func (G_OBJECT (ginfo -> gui_info.title_edit_entry),
	                                   on_title_edit_changed, (gpointer) ginfo);
}

void set_artist (GripInfo *ginfo, char *artist) {
	g_signal_handlers_block_by_func (G_OBJECT (ginfo -> gui_info.artist_edit_entry),
	                                 on_artist_edit_changed, (gpointer) ginfo);

	gtk_entry_set_text (GTK_ENTRY (ginfo -> gui_info.artist_edit_entry), artist);
	gtk_entry_set_position (GTK_ENTRY (ginfo -> gui_info.artist_edit_entry), 0);

	strcpy (ginfo -> ddata.data_artist, artist);
	gtk_label_set (GTK_LABEL (ginfo -> gui_info.disc_artist_label), artist);

	g_signal_handlers_unblock_by_func (G_OBJECT (ginfo -> gui_info.artist_edit_entry),
	                                   on_artist_edit_changed, (gpointer) ginfo);
}

void set_year (GripInfo *ginfo, int year) {
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ginfo -> gui_info.year_spin_button),
	                           (gfloat) year);
}

void set_genre (GripInfo *ginfo, char *genre) {
	GripGUI *uinfo = &(ginfo -> gui_info);

    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (uinfo -> genre_combo))), genre);
}

static void save_disc_info (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	if (ginfo -> have_disc) {
//		if (DiscDBWriteDiscData (&(ginfo -> disc), &(ginfo -> ddata), NULL, TRUE, FALSE,
//		                         "utf-8") < 0)
//			show_warning (ginfo -> gui_info.app,
//			              _("Error saving disc data."));
	} else show_warning (ginfo -> gui_info.app,
		                     _("No disc present."));
}

static void on_title_edit_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	strcpy (ginfo -> ddata.data_title,
	        gtk_entry_get_text (GTK_ENTRY (ginfo -> gui_info.title_edit_entry)));

	gtk_label_set (GTK_LABEL (ginfo -> gui_info.disc_name_label),
	               ginfo -> ddata.data_title);
}

static void on_artist_edit_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	strcpy (ginfo -> ddata.data_artist,
	        gtk_entry_get_text (GTK_ENTRY (ginfo -> gui_info.artist_edit_entry)));

	gtk_label_set (GTK_LABEL (ginfo -> gui_info.disc_artist_label),
	               ginfo -> ddata.data_artist);
}

static void on_year_edit_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	ginfo -> ddata.data_year =
	    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ginfo -> gui_info.
	                                      year_spin_button));
}

void on_track_edit_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
	char newname[MAX_STRING];
	GtkTreeIter iter;
	gint i;

	ginfo = (GripInfo *) data;

	strcpy (ginfo -> ddata.data_track[CURRENT_TRACK].track_name,
	        gtk_entry_get_text (GTK_ENTRY (ginfo -> gui_info.track_edit_entry)));

	strcpy (ginfo -> ddata.data_track[CURRENT_TRACK].track_artist,
	        gtk_entry_get_text (GTK_ENTRY (ginfo -> gui_info.track_artist_edit_entry)));

	if (*ginfo -> ddata.data_track[CURRENT_TRACK].track_artist)
		g_snprintf (newname, MAX_STRING, "%02d  %s (%s)", CURRENT_TRACK + 1,
		            ginfo -> ddata.data_track[CURRENT_TRACK].track_name,
		            ginfo -> ddata.data_track[CURRENT_TRACK].track_artist);
	else
		g_snprintf (newname, MAX_STRING, "%02d  %s", CURRENT_TRACK + 1,
		            ginfo -> ddata.data_track[CURRENT_TRACK].track_name);

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (
	                                   ginfo -> gui_info.track_list_store),
	                               &iter);

	for (i = 0; i < CURRENT_TRACK; i++)
		gtk_tree_model_iter_next (GTK_TREE_MODEL (ginfo -> gui_info.track_list_store),
		                          &iter);

// FIXME
//	gtk_list_store_set (ginfo -> gui_info.track_list_store, &iter,
//	                    TRACKLIST_TRACK_COL, newname, -1);
	/*  gtk_clist_set_text(GTK_CLIST(ginfo -> gui_info.trackclist),
	    CURRENT_TRACK,0,newname);*/
}

static void edit_next_track (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	next_track (ginfo);
	/*  gtk_editable_select_region(GTK_EDITABLE(track_edit_entry),0,-1);*/
	gtk_widget_grab_focus (GTK_WIDGET (ginfo -> gui_info.track_edit_entry));
}

static void on_genre_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo = (GripInfo *) data;
	GripGUI *uinfo = &(ginfo -> gui_info);

	const char *genre = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (uinfo -> genre_combo))));
	strncpy (ginfo -> ddata.data_genre, genre, MAX_STRING);
}

static void separate_fields (char *buf, char *field1, char *field2, char *sep) {
	char *tmp;
	char spare[80];

	tmp = strtok (buf, sep);

	if (!tmp) {
		return;
	}

	strncpy (spare, g_strstrip (tmp), 80);

	tmp = strtok (NULL, "");

	if (tmp) {
		strncpy (field2, g_strstrip (tmp), 80);
	} else {
		*field2 = '\0';
	}

	strcpy (field1, spare);
}

static void split_title_artist (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
	int track;
	int mode;

	ginfo = (GripInfo *) data;
	mode = GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (widget)));

	for (track = 0; track < ginfo -> disc.num_tracks; track++) {
		if (mode == 0)
			separate_fields (ginfo -> ddata.data_track[track].track_name,
			                ginfo -> ddata.data_track[track].track_name,
			                ginfo -> ddata.data_track[track].track_artist,
			                ginfo -> title_split_chars);
		else
			separate_fields (ginfo -> ddata.data_track[track].track_name,
			                ginfo -> ddata.data_track[track].track_artist,
			                ginfo -> ddata.data_track[track].track_name,
			                ginfo -> title_split_chars);
	}

	update_tracks (ginfo);
}

static void on_submit_entry (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
	int len;

	ginfo = (GripInfo *)data;

	if (!ginfo -> have_disc) {
		show_warning (ginfo -> gui_info.app,
		              _("Cannot submit. No disc is present."));

		return;
	}

	if (!ginfo -> ddata.data_genre) {
		/*    gnome_app_warning((GnomeApp *)ginfo -> gui_info.app,
		      _("Submission requires a genre other than 'unknown'."));*/
		get_discDBGenre (ginfo);

		return;
	}

	if (!*ginfo -> ddata.data_title) {
		show_warning (ginfo -> gui_info.app,
		              _("You must enter a disc title."));

		return;
	}

	if (!*ginfo -> ddata.data_artist) {
		show_warning (ginfo -> gui_info.app,
		              _("You must enter a disc artist."));

		return;
	}

	len = strlen (ginfo -> discdb_submit_email);

	if (strncasecmp (ginfo  ->  discdb_submit_email + (len - 9), ".cddb.com",
	                 9) == 0) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (ginfo -> gui_info.app),
		                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                    GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
		                    _("You are about to submit this disc information\n"
		                       "to a commercial CDDB server, which will then\n"
		                       "own the data that you submit. These servers make\n"
		                       "a profit out of your effort. We suggest that you\n"
		                       "support free servers instead.\n\nContinue?"));
//	gtk_window_set_title (GTK_WINDOW (dialog), "Warning");
		g_signal_connect (dialog,
		                  "response",
		                  G_CALLBACK (submit_entry),
		                  ginfo);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (ginfo -> gui_info.app),
		                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                    GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
		                    _("You are about to submit this\ndisc information via email.\n\n"
		                       "Continue?"));
//	gtk_window_set_title (GTK_WINDOW (dialog), "Warning");
		g_signal_connect (dialog,
		                  "response",
		                  G_CALLBACK (submit_entry),
		                  ginfo);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

/* Make the user pick a DiscDB genre on submit*/
static void get_discDBGenre (GripInfo *ginfo) {
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *submit_button;
	GtkWidget *cancel_button;
	GtkWidget *hbox;
	GtkWidget *genre_combo;
//	GtkWidget *item;
//	int genre;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), _("Genre selection"));

	gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog) -> vbox), 5);

	label = gtk_label_new (_("Submission requires a genre other than 'unknown'\n"
	                          "Please select a DiscDB genre below"));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog) -> vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	genre_combo = gtk_combo_new();
	gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (genre_combo) -> entry), FALSE);

	hbox = gtk_hbox_new (FALSE, 3);

	label = gtk_label_new (_("DiscDB genre"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	// FIXME
//	for (genre = 0; genre < 12; genre++) {
//		item = gtk_list_item_new_with_label (DiscDBGenre (genre));
//		gtk_object_set_user_data (GTK_OBJECT (item),
//		                          GINT_TO_POINTER (genre));
//		gtk_signal_connect (GTK_OBJECT (item), "select",
//		                    GTK_SIGNAL_FUNC (DiscDBon_genre_changed), (gpointer)ginfo);
//		gtk_container_add (GTK_CONTAINER (GTK_COMBO (genre_combo) -> list), item);
//		gtk_widget_show (item);
//	}

	gtk_box_pack_start (GTK_BOX (hbox), genre_combo, TRUE, TRUE, 0);
	gtk_widget_show (genre_combo);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog) -> vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	submit_button = gtk_button_new_with_label (_("Submit"));

	gtk_signal_connect (GTK_OBJECT (submit_button), "clicked",
	                    (gpointer)on_submit_entry, (gpointer)ginfo);
	gtk_signal_connect_object (GTK_OBJECT (submit_button), "clicked",
	                           GTK_SIGNAL_FUNC (gtk_widget_destroy),
	                           GTK_OBJECT (dialog));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog) -> action_area), submit_button,
	                    TRUE, TRUE, 0);
	gtk_widget_show (submit_button);

	cancel_button = gtk_button_new_with_label (_("Cancel"));

	gtk_signal_connect_object (GTK_OBJECT (cancel_button), "clicked",
	                           GTK_SIGNAL_FUNC (gtk_widget_destroy),
	                           GTK_OBJECT (dialog));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog) -> action_area), cancel_button,
	                    TRUE, TRUE, 0);
	gtk_widget_show (cancel_button);

	gtk_widget_show (dialog);

	gtk_grab_add (dialog);
}

#if 0
/* Set the DiscDB genre when a combo item is selected */
static void DiscDBon_genre_changed (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo = (GripInfo *) data;
	GripGUI *uinfo = &(ginfo -> gui_info);

	char *genre = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (uinfo -> genre_combo))));
	strncpy (ginfo -> ddata.data_genre, genre, MAX_STRING);
	// genre must not be freed
}
#endif
