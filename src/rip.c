/* rip.c
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

 #include <stdlib.h>
#include "grip.h"
#include <sys/stat.h>
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#elif defined (HAVE_SYS_VFS_H)
#include <sys/vfs.h>
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <sys/types.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include "rip.h"
#include "dialog.h"
#include "cdplay.h"
#include "cddev.h"
#include "gripcfg.h"
#include "launch.h"
#include "tag.h"
#include "config.h"
#include "uihelper.h"
#include "gain_analysis.h"
#include "cdpar.h"
#include "encoder.h"


static void PlaySegmentCB (GtkWidget *widget, gpointer data);
static char *MakeRelative (char *file1, char *file2);
static gboolean AddM3U (GripInfo *ginfo);
static void ID3Add (GripInfo *ginfo, char *file, EncodeTrack *enc_track);
static void DoWavFilter (GripInfo *ginfo);
static void DoDiscFilter (GripInfo *ginfo);
static void RipIsFinished (GripInfo *ginfo, gboolean aborted);
static void CheckDupNames (GripInfo *ginfo);
static void RipWholeCD (GtkDialog *dialog, gint reply, gpointer data);
static int NextTrackToRip (GripInfo *ginfo);
static gboolean RipNextTrack (GripInfo *ginfo);
static void CalculateAll (GripInfo *ginfo);
static size_t CalculateEncSize (GripInfo *ginfo, int track);
static size_t CalculateWavSize (GripInfo *ginfo, int track);

#if 0
void MakeRipPage (GripInfo *ginfo) {
	GripGUI *uinfo;
	GtkWidget *rippage;
	GtkWidget *rangesel;
	GtkWidget *vbox, *vbox2, *hbox, *hbox2;
	GtkWidget *button;
	GtkWidget *hsep;
	GtkWidget *check;
	GtkWidget *partial_rip_frame;
	int label_width;
	PangoLayout *layout;

	uinfo = & (ginfo -> gui_info);

	rippage = MakeNewPage (uinfo -> notebook, _("Rip"));

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	hbox = gtk_hbox_new (FALSE, 5);

	vbox2 = gtk_vbox_new (FALSE, 0);

	button = gtk_button_new_with_label (_("Rip!"));
	gtk_tooltips_set_tip (MakeToolTip(), button,
						  _("Rip selected tracks"), NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
						GTK_SIGNAL_FUNC (DoRip), (gpointer) ginfo);
	gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("Abort Rip"));
	gtk_tooltips_set_tip (MakeToolTip(), button,
						  _("Kill rip process"), NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
						GTK_SIGNAL_FUNC (KillRip), (gpointer) ginfo);
	gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	gtk_widget_show (vbox2);

	partial_rip_frame = gtk_frame_new (NULL);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox2), 3);

	check = MakeCheckButton (NULL, &ginfo -> rip_partial, _("Rip partial track"));
	gtk_signal_connect (GTK_OBJECT (check), "clicked",
						GTK_SIGNAL_FUNC (RipPartialChanged), (gpointer) ginfo);
	gtk_box_pack_start (GTK_BOX (vbox2), check, FALSE, FALSE, 0);
	gtk_widget_show (check);

	uinfo -> partial_rip_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_sensitive (uinfo -> partial_rip_box, ginfo -> rip_partial);

	hbox2 = gtk_hbox_new (FALSE, 5);

	button = gtk_button_new_with_label (_("Play"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
						GTK_SIGNAL_FUNC (PlaySegmentCB),
						(gpointer) ginfo);
	gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	uinfo -> play_sector_label = gtk_label_new (_("Current sector:      0"));
	gtk_box_pack_start (GTK_BOX (hbox2), uinfo -> play_sector_label, FALSE, FALSE, 0);
	gtk_widget_show (uinfo -> play_sector_label);

	gtk_box_pack_start (GTK_BOX (uinfo -> partial_rip_box), hbox2, FALSE, FALSE, 0);
	gtk_widget_show (hbox2);

	rangesel = MakeRangeSelects (ginfo);
	gtk_box_pack_start (GTK_BOX (uinfo -> partial_rip_box), rangesel, FALSE, FALSE,
						0);
	gtk_widget_show (rangesel);

	gtk_box_pack_start (GTK_BOX (vbox2), uinfo -> partial_rip_box, TRUE, TRUE, 0);
	gtk_widget_show (uinfo -> partial_rip_box);

	gtk_container_add (GTK_CONTAINER (partial_rip_frame), vbox2);
	gtk_widget_show (vbox2);

	gtk_box_pack_start (GTK_BOX (hbox), partial_rip_frame, TRUE, TRUE, 0);
	gtk_widget_show (partial_rip_frame);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	hsep = gtk_hseparator_new();
	gtk_box_pack_start (GTK_BOX (vbox), hsep, TRUE, TRUE, 0);
	gtk_widget_show (hsep);

	hbox = gtk_hbox_new (FALSE, 3);

	uinfo -> rip_prog_label = gtk_label_new (_("Rip: Idle"));

	/* This should be the largest this string can get */
	layout = gtk_widget_create_pango_layout (GTK_WIDGET (uinfo -> app),
			 _("Enc: Trk 99 (99.9x)"));
	pango_layout_get_size (layout, &label_width, NULL);
	label_width /= PANGO_SCALE;
	g_object_unref (layout);


	gtk_widget_set_usize (uinfo -> rip_prog_label, label_width, 0);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> rip_prog_label, FALSE, FALSE, 0);
	gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), _("Rip: Idle"));
	gtk_widget_show (uinfo -> rip_prog_label);

	uinfo -> ripprogbar = gtk_progress_bar_new();
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> ripprogbar, FALSE, FALSE, 0);
	gtk_widget_show (uinfo -> ripprogbar);

	uinfo -> smile_indicator = NewBlankPixmap (uinfo -> app);
	gtk_tooltips_set_tip (MakeToolTip(), uinfo -> smile_indicator,
						  _("Rip status"), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> smile_indicator, FALSE, FALSE, 0);
	gtk_widget_show (uinfo -> smile_indicator);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
	gtk_widget_show (hbox);

	/*
		hbox = gtk_hbox_new (FALSE, 3);

		uinfo -> mp3_prog_label = gtk_label_new (_("Enc: Idle"));
		gtk_widget_set_usize (uinfo -> mp3_prog_label, label_width, 0);

		gtk_box_pack_start (GTK_BOX (hbox), uinfo -> mp3_prog_label,
							FALSE, FALSE, 0);
		gtk_widget_show (uinfo -> mp3_prog_label);

		uinfo -> mp3progbar = gtk_progress_bar_new();

		gtk_box_pack_start (GTK_BOX (hbox), uinfo -> mp3progbar, FALSE, FALSE, 0);
		gtk_widget_show (uinfo -> mp3progbar);

		gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
		gtk_widget_show (hbox);

		gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
		gtk_widget_show (vbox2);
	*/

//	hsep = gtk_hseparator_new ();
//	gtk_box_pack_start (GTK_BOX (vbox), hsep, TRUE, TRUE, 0);
//	gtk_widget_show (hsep);

//	vbox2 = gtk_vbox_new (FALSE, 0);
//	uinfo -> all_label = gtk_label_new (_("Overall indicators:"));
//	gtk_box_pack_start (GTK_BOX (vbox2), uinfo -> all_label, FALSE, FALSE, 0);
//	gtk_widget_show (uinfo -> all_label);

	hbox = gtk_hbox_new (FALSE, 2);
	uinfo -> all_rip_label = gtk_label_new (_("Overall: Idle"));
	gtk_widget_set_usize (uinfo -> all_rip_label, label_width, 0);
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> all_rip_label, FALSE, FALSE, 0);
	gtk_widget_show (uinfo -> all_rip_label);

	uinfo -> all_ripprogbar = gtk_progress_bar_new();
	gtk_box_pack_start (GTK_BOX (hbox), uinfo -> all_ripprogbar, FALSE, FALSE, 0);
	gtk_widget_show (uinfo -> all_ripprogbar);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
	gtk_widget_show (hbox);

	/*
		hbox = gtk_hbox_new (FALSE, 2);
		uinfo -> all_enc_label = gtk_label_new (_("Enc: Idle"));
		gtk_widget_set_usize (uinfo -> all_enc_label, label_width, 0);
		gtk_box_pack_start (GTK_BOX (hbox), uinfo -> all_enc_label, FALSE, FALSE, 0);
		gtk_widget_show (uinfo -> all_enc_label);

		uinfo -> all_encprogbar = gtk_progress_bar_new();
		gtk_box_pack_start (GTK_BOX (hbox), uinfo -> all_encprogbar, FALSE, FALSE, 0);
		gtk_widget_show (uinfo -> all_encprogbar);

		gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
		gtk_widget_show (hbox);

		gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
		gtk_widget_show (vbox2);
	*/

	gtk_container_add (GTK_CONTAINER (rippage), vbox);
	gtk_widget_show (vbox);
}

static void RipPartialChanged (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	gtk_widget_set_sensitive (ginfo -> gui_info.partial_rip_box, ginfo -> rip_partial);
}
#endif

unsigned long long BytesLeftInFS (char *path) {
	unsigned long long bytesleft;
	int pos;
#ifdef HAVE_SYS_STATVFS_H
	struct statvfs stat;
#else
	struct statfs stat;
#endif

	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		for (pos = strlen (path); pos && (path[pos] != '/'); pos--);

		if (path[pos] != '/') {
			return 0;
		}

		path[pos] = '\0';

#ifdef HAVE_SYS_STATVFS_H

		if (statvfs (path, &stat) != 0) {
			return 0;
		}

#else

		if (statfs (path, &stat) != 0) {
			return 0;
		}

#endif

		path[pos] = '/';
	} else
#ifdef HAVE_SYS_STATVFS_H
		if (statvfs (path, &stat) != 0) {
			return 0;
		}

#else

		if (statfs (path, &stat) != 0) {
			return 0;
		}

#endif

	bytesleft = stat.f_bavail;
	bytesleft *= stat.f_bsize;

	return bytesleft;
}

// FIXME I think this can be replaced with basename()
/* Find the root filename of a path */
char *FindRoot (char *str) {
	char *c;

	for (c = str + strlen (str); c > str; c--) {
		if (*c == '/') {
			return c + 1;
		}
	}

	return c;
}

/* Make file1 relative to file2 */
static char *MakeRelative (char *file1, char *file2) {
	int pos, pos2 = 0, slashcnt, i;
	char *rel = file1;
	char tem[PATH_MAX] = "";

	slashcnt = 0;

	/* This part finds relative names assuming m3u is not branched in a
	   different directory from mp3 */
	for (pos = 0; file2[pos]; pos++) {
		if (pos && (file2[pos] == '/')) {
			if (!strncmp (file1, file2, pos)) {
				rel = file1 + pos + 1;
				pos2 = pos;
			}
		}
	}

	/* Now check to see if the m3u file branches to a different directory. */
	for (pos2 = pos2 + 1; file2[pos2]; pos2++) {
		if (file2[pos2] == '/') {
			slashcnt++;
		}
	}

	/* Now add correct number of "../"s to make the path relative */
	for (i = 0; i < slashcnt; i++) {
		strcpy (tem, "../");
		strncat (tem, rel, strlen (rel));
		strcpy (rel, tem);
	}

	return rel;
}

static gboolean AddM3U (GripInfo *ginfo) {
	int i;
	EncodeTrack enc_track;
	FILE *fp;
	char tmp[PATH_MAX];
	char m3unam[PATH_MAX];
	char *relnam;
	GString *str;
	char *conv_str;
	gsize rb, wb;

	if (!ginfo -> have_disc) {
		return FALSE;
	}

	str = g_string_new (NULL);

	/* Use track 0 to fill in M3u switches */
	FillInTrackInfo (ginfo, 0, &enc_track);

	TranslateString (ginfo -> m3ufileformat, str, TranslateSwitch,
					 &enc_track, TRUE, & (ginfo -> sprefs));

	conv_str = g_filename_from_utf8 (str -> str, strlen (str -> str), &rb, &wb, NULL);
	if (!conv_str) {
		conv_str = g_strdup (str -> str);
	}

	g_snprintf (m3unam, PATH_MAX, "%s", conv_str);

	gchar *dir = g_path_get_dirname (conv_str);
	if (dir == NULL || g_mkdir_with_parents (dir, 0755) < 0) {
		show_error (ginfo -> gui_info.app,
					_("Error: can't create directories for m3u file."));
		g_free (dir);
		g_free (conv_str);
		return FALSE;
	}
	g_free (dir);

	fp = fopen (conv_str, "w");
	if (fp == NULL) {
		show_error (ginfo -> gui_info.app,
					_("Error: can't open m3u file."));
		g_free (conv_str);
		return FALSE;
	}

	g_free (conv_str);

	for (i = 0; i < ginfo -> disc.num_tracks; i++) {
		/* Only add to the m3u if the track is selected for ripping */
		if (TrackIsChecked (& (ginfo -> gui_info), i)) {
			g_string_truncate (str, 0);

			FillInTrackInfo (ginfo, i, &enc_track);
//			TranslateString (ginfo -> mp3fileformat, str, TranslateSwitch,
//			                 &enc_track, TRUE, &(ginfo -> sprefs));

			conv_str = g_filename_from_utf8 (str -> str, strlen (str -> str), &rb, &wb, NULL);

			if (!conv_str) {
				conv_str = g_strdup (str -> str);
			}

			if (ginfo -> rel_m3u) {
				g_snprintf (tmp, PATH_MAX, "%s", conv_str);
				relnam = MakeRelative (tmp, m3unam);
				fprintf (fp, "%s\n", relnam);
			} else {
				fprintf (fp, "%s\n", conv_str);
			}

			g_free (conv_str);
		}
	}

	g_string_free (str, TRUE);

	fclose (fp);

	return TRUE;
}

void KillRip (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;
//	int track;

	g_debug (_("In KillRip"));
	ginfo = (GripInfo *) data;

	if (!ginfo -> ripping) {
		return;
	}

	ginfo -> all_ripsize = 0;
	ginfo -> all_ripdone = 0;
	ginfo -> all_riplast = 0;

	ginfo -> stop_rip = TRUE;
	ginfo -> ripping = FALSE;

	// FIXME: What is this doing? Looks unnecessary nowadays.
	/* Need to decrement all_mp3size */
//    for (track = 0; track < ginfo -> disc.num_tracks; ++track) {
//        if ((!IsDataTrack (&(ginfo -> disc), track)) &&
//                (TrackIsChecked (&(ginfo -> gui_info), track))) {
//            ginfo -> all_encsize -= CalculateEncSize (ginfo, track);
//        }
//    }

	g_debug (_("Now total enc size is: %zu"), ginfo -> all_encsize);
}

static void ID3Add (GripInfo *ginfo, char *file, EncodeTrack *enc_track) {
	if (ginfo -> doid3) {
		GString *comment = g_string_new (NULL);
//        g_debug ("comment = '%s'", ginfo -> id3_comment);
		TranslateString (ginfo -> id3_comment, comment, TranslateSwitch, enc_track,
						 FALSE, & (ginfo -> sprefs));
		g_assert (comment);

		GError *error = NULL;
		if (!tag_file (file, (*(enc_track -> song_name)) ? enc_track -> song_name :
					   "Unknown",
					   (*(enc_track -> song_artist)) ? enc_track -> song_artist :
					   (*(enc_track -> disc_artist)) ? enc_track -> disc_artist : "Unknown",
					   (*(enc_track -> disc_name)) ? enc_track -> disc_name : "Unknown",
					   enc_track -> song_year, comment -> str, enc_track -> genre,
					   enc_track -> track_num + 1, ginfo -> id3_encoding, &error)) {

			// Cannot tag file
			show_error (ginfo -> gui_info.app, error -> message);
			g_error_free (error);
		}

		g_string_free (comment, TRUE);
	}
}

static void DoWavFilter (GripInfo *ginfo) {
	EncodeTrack enc_track;

	FillInTrackInfo (ginfo, ginfo -> rip_track, &enc_track);
	strcpy (enc_track.wav_filename, ginfo -> ripfile);

	TranslateAndLaunch (ginfo -> wav_filter_cmd, TranslateSwitch, &enc_track,
						FALSE, &(ginfo -> sprefs), CloseStuff, (void *) ginfo);
}

static void DoDiscFilter (GripInfo *ginfo) {
	EncodeTrack enc_track;

	FillInTrackInfo (ginfo, ginfo -> rip_track, &enc_track);
	strcpy (enc_track.wav_filename, ginfo -> ripfile);

	TranslateAndLaunch (ginfo -> disc_filter_cmd, TranslateSwitch, &enc_track,
						FALSE, &(ginfo -> sprefs), CloseStuff, (void *) ginfo);
}

void UpdateRipProgress (GripInfo *ginfo) {
	GripGUI *uinfo;
	struct stat mystat;
	int quarter;
	gfloat percent = 0;
	char buf[PATH_MAX];
	time_t now;
	gfloat elapsed = 0;
	gfloat speed;

	uinfo = & (ginfo -> gui_info);

	if (ginfo -> ripping) {
		// Rip percent is calculated by the cdparanoia callback and provided to rip_callback().
        g_assert (uinfo -> ripprogbar);
		gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> ripprogbar), (gdouble) ginfo -> rip_percent);
		gchar *tmp = g_strdup_printf ("%.0f%%", ginfo -> rip_percent * 100);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (uinfo -> ripprogbar), tmp);
		g_free (tmp);

		now = time (NULL);
		elapsed = (gfloat) now - (gfloat) ginfo -> rip_started;

		/* 1x is 44100*2*2 = 176400 bytes/sec */
		if (elapsed < 0.1f) { /* 1/10 sec. */
			speed = 0.0f;    /* Avoid divide-by-0 at start */
		} else {
			speed = (gfloat) (mystat.st_size) / (176400.0f * elapsed);
		}

		/* startup */
		if (speed >= 50.0f) {
			speed = 0.0f;
			ginfo -> rip_started = now;
		}

		sprintf (buf, _("Rip: Trk %d (%3.1fx)"), ginfo -> rip_track + 1, speed);

		gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), buf);

		quarter = (int) (ginfo -> rip_percent * 4.0);

		if (quarter < 4)
			CopyPixmap (GTK_PIXMAP (uinfo -> rip_pix[quarter]),
						GTK_PIXMAP (uinfo -> rip_indicator));

		int slevel;
		if (ginfo -> rip_smile_level < 8 && ginfo -> rip_smile_level > 0) {
			slevel = ginfo -> rip_smile_level - 1;
		} else {
			slevel = 0;
		}
		if (uinfo -> minimized)
			CopyPixmap (GTK_PIXMAP (uinfo -> smile_pix[slevel]),
						GTK_PIXMAP (uinfo -> lcd_smile_indicator));
		else
			CopyPixmap (GTK_PIXMAP (uinfo -> smile_pix[slevel]),
						GTK_PIXMAP (uinfo -> smile_indicator));

		/* Overall rip */
		if (ginfo -> rip_started != now && !ginfo -> rip_partial && ginfo -> ripping
				&& !ginfo -> stop_rip) {
			ginfo -> all_ripdone += mystat.st_size - ginfo -> all_riplast;
			ginfo -> all_riplast = mystat.st_size;
			percent = (gfloat) (ginfo -> all_ripdone) / (gfloat) (ginfo -> all_ripsize);

			if (percent > 1.0) {
				percent = 0.0;
			}

			ginfo -> rip_tot_percent = percent;

			sprintf (buf, _("Rip: %6.2f%%"), percent * 100.0);
			// FIXME
//			gtk_label_set (GTK_LABEL (uinfo -> all_rip_label), buf);
//			gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> all_ripprogbar), percent);
		} else if (ginfo -> stop_rip) {
//			gtk_label_set (GTK_LABEL (uinfo -> all_rip_label), _("Rip: Idle"));

//			gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> all_ripprogbar), 0.0);
		}

		/* Check if a rip finished */
		if (!ginfo -> in_rip_thread) {
			// Clear thread struct
			gboolean rip_ok = GPOINTER_TO_INT (g_thread_join (ginfo -> rip_thread));
			if (rip_ok) {
				g_debug (_("Rip thread finished successfully"));
			} else {
				g_debug (_("Rip thread finished with failure or was aborted"));
			}
			ginfo -> rip_thread = NULL;

			CopyPixmap (GTK_PIXMAP (uinfo -> empty_image),
						GTK_PIXMAP (uinfo -> lcd_smile_indicator));
			CopyPixmap (GTK_PIXMAP (uinfo -> empty_image),
						GTK_PIXMAP (uinfo -> smile_indicator));

			// Terminate encoding
			g_assert (ginfo -> encoder);
			GError *error = NULL;
			ginfo -> encoder -> close (ginfo -> encoder_data, &error);
			// FIXME Handle error

			// Close and rename temporary file
//            fclose ()
			close (ginfo -> riptmpfd);

			if (rip_ok) {
				g_rename (ginfo -> rip_tmpfile, ginfo -> ripfile);

				ginfo -> all_riplast = 0;
				ginfo -> ripping = FALSE;
				SetChecked (uinfo, ginfo -> rip_track, FALSE);

				/* Get the title gain */
				if (ginfo -> calc_gain) {
					ginfo -> track_gain_adjustment = GetTitleGain ();
				}

				/* Apply tag */
				EncodeTrack enc_track;
				FillInTrackInfo (ginfo, ginfo -> rip_track, &enc_track);
				strcpy (enc_track.wav_filename, ginfo -> ripfile);  // FIXME: probably useless
				ID3Add (ginfo, ginfo -> ripfile, &enc_track);

				/* Do filtering of .wav file */
				if (*ginfo -> wav_filter_cmd) {
					DoWavFilter (ginfo);
				}
			} else {
				// Remove temporary file
				g_unlink (ginfo -> rip_tmpfile);
				(ginfo -> rip_tmpfile) [0] = '\0';

				show_error (ginfo -> gui_info.app, "Rip failed");
			}

			gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> ripprogbar), 0.0);
			CopyPixmap (GTK_PIXMAP (uinfo -> empty_image),
						GTK_PIXMAP (uinfo -> rip_indicator));

			if (!ginfo -> stop_rip) {
				g_debug (_("Rip partial %d"), ginfo -> rip_partial);

				g_debug (_("Next track is %d, total is %d"),
						 NextTrackToRip (ginfo), ginfo -> disc.num_tracks);

				if (!ginfo -> rip_partial &&
						(NextTrackToRip (ginfo) == ginfo -> disc.num_tracks)) {
					g_debug (_("Check if we need to rip another track"));

					if (!RipNextTrack (ginfo)) {
						RipIsFinished (ginfo, FALSE);
					} else {
						gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), _("Rip: Idle"));
					}
				} else {
					gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), _("Rip: Idle"));
				}
			} else {
				RipIsFinished (ginfo, TRUE);
			}
		}
	} else {
		if (ginfo -> stop_rip) {
			RipIsFinished (ginfo, TRUE);
		}
	}

	/* Check if an encode finished */
#if 0
	if (ginfo -> encoding & (1 << mycpu)) {
//        if (stat (ginfo -> mp3file, &mystat) >= 0) {
//            percent = (gfloat)mystat.st_size / (gfloat)ginfo -> mp3size[mycpu];
//
//            if (percent > 1.0) {
//                percent = 1.0;
//            }
//        } else {
//            percent = 0;
//        }
//
//        ginfo -> enc_percent = percent;
//
//        gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> mp3progbar[mycpu]),
//                                 percent);

		now = time (NULL);
		elapsed = (gfloat) now - (gfloat) ginfo -> mp3_started;

		// FIXME
//			if (elapsed < 0.1f) { /* 1/10 sec. */
		speed = 0.0f;
//			} else
//				speed = (gfloat)mystat.st_size /
//				        ((gfloat)ginfo -> kbits_per_sec * 128.0f * elapsed);

		sprintf (buf, _("Enc: Trk %d (%3.1fx)"),
				 ginfo -> mp3_enc_track + 1, speed);

		gtk_label_set (GTK_LABEL (uinfo -> mp3_prog_label), buf);

		quarter = (int) (percent * 4.0);

		if (quarter < 4)
			CopyPixmap (GTK_PIXMAP (uinfo -> mp3_pix[quarter]),
						GTK_PIXMAP (uinfo -> mp3_indicator));

		if (!ginfo -> rip_partial && !ginfo -> stop_encode &&
				now != ginfo -> mp3_started) {
			ginfo -> all_encdone += mystat.st_size - ginfo -> all_enclast;
			ginfo -> all_enclast[mycpu] = mystat.st_size;
			percent = (gfloat) (ginfo -> all_encdone) / (gfloat) (ginfo -> all_encsize);

			if (percent > 1.0) {
				percent = 1.0;
			}

			ginfo -> enc_tot_percent = percent;
			sprintf (buf, _("Enc: %6.2f%%"), percent * 100.0);
			gtk_label_set (GTK_LABEL (uinfo -> all_enc_label), buf);
			gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> all_encprogbar),
									 percent);
		}

		if (waitpid (ginfo -> mp3pid[mycpu], NULL, WNOHANG)) {
			waitpid (ginfo -> mp3pid[mycpu], NULL, 0);
			ginfo -> encoding &= ~ (1 << mycpu);

			g_message (ginfo, _("Finished encoding on cpu %d\n"), mycpu);
			ginfo -> all_enclast[mycpu] = 0;
			gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> mp3progbar[mycpu]),
									 0.0);
			CopyPixmap (GTK_PIXMAP (uinfo -> empty_image),
						GTK_PIXMAP (uinfo -> mp3_indicator[mycpu]));

			if (!ginfo -> stop_encode) {
				if (ginfo -> doid3)
					ID3Add (ginfo, ginfo -> mp3file[mycpu],
							ginfo -> encoded_track[mycpu]);

				if (*ginfo -> mp3_filter_cmd)
					TranslateAndLaunch (ginfo -> mp3_filter_cmd, TranslateSwitch,
										ginfo -> encoded_track[mycpu], FALSE,
										& (ginfo -> sprefs), CloseStuff, (void *) ginfo);


				if (ginfo -> ripping_a_disc && !ginfo -> rip_partial &&
						!ginfo -> ripping) {
					if (RipNextTrack (ginfo)) {
//							ginfo -> doencode = TRUE;
					} else {
						RipIsFinished (ginfo, FALSE);
					}
				}

				g_free (ginfo -> encoded_track[mycpu]);

//					if (!ginfo -> rip_partial && ginfo -> encode_list) {
//						MP3Encode (ginfo);
//					}
			} else {
				ginfo -> stop_encode = FALSE;
			}

			if (! (ginfo -> encoding & (1 << mycpu)) ) {
				gtk_label_set (GTK_LABEL (uinfo -> mp3_prog_label[mycpu]),
							   _("Enc: Idle"));
			}
		}
	}

	if (!ginfo -> ripping) {
		gtk_label_set (GTK_LABEL (uinfo -> all_enc_label), _("Enc: Idle"));
		gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> all_encprogbar), 0.0);
	}
#endif
}

static void RipIsFinished (GripInfo *ginfo, gboolean aborted) {
	GripGUI *uinfo = &(ginfo -> gui_info);
	ginfo -> all_ripsize = 0;
	ginfo -> all_ripdone = 0;
	ginfo -> all_riplast = 0;

	g_message ("Ripping complete");

//	gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), _("Rip: Idle"));
//	gtk_label_set (GTK_LABEL (uinfo -> all_rip_label), _("Overall: Idle"));
//	gtk_progress_bar_update (GTK_PROGRESS_BAR (uinfo -> all_ripprogbar), 0.0);

	gtk_widget_hide (uinfo -> rip_win);

	ginfo -> stop_rip = FALSE;
	ginfo -> ripping = FALSE;
	ginfo -> rip_finished = time (NULL);

	/* Re-open the cdrom device if it was closed */
	if (ginfo -> disc.cd_desc < 0) {
		CDInitDevice (ginfo -> disc.devname, & (ginfo -> disc));
	}

	/* Do post-rip stuff only if we weren't explicitly aborted */
	if (!aborted) {
		if (ginfo -> beep_after_rip) {
			printf ("%c%c", 7, 7);
		}

		if (ginfo -> calc_gain) {
			ginfo -> disc_gain_adjustment = GetAlbumGain();
		}

		if (*ginfo -> disc_filter_cmd) {
			DoDiscFilter (ginfo);
		}

		if (ginfo -> eject_after_rip) {
			/* Reset rip_finished since we're ejecting */
			ginfo -> rip_finished = 0;

			EjectDisc (NULL, ginfo);

			if (ginfo -> eject_delay) {
				ginfo -> auto_eject_countdown = ginfo -> eject_delay;
			}
		}
	}
}

char *TranslateSwitch (char switch_char, void *data, gboolean *munge) {
	static char res[PATH_MAX];
	EncodeTrack *enc_track;

	enc_track = (EncodeTrack *) data;

	switch (switch_char) {
//		case 'b':
//			g_snprintf (res, PATH_MAX, "%d", enc_track -> ginfo -> kbits_per_sec);
//			*munge = FALSE;
//			break;

	case 'c':
		g_snprintf (res, PATH_MAX, "%s", enc_track -> ginfo -> cd_device);
		*munge = FALSE;
		break;

	case 'C':
		if (*enc_track -> ginfo -> force_scsi) {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> ginfo -> force_scsi);
		} else {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> ginfo -> cd_device);
		}

		*munge = FALSE;
		break;

//	case 'w':
//		g_snprintf (res, PATH_MAX, "%s", enc_track -> wav_filename);
//		*munge = FALSE;
//		break;
//
//	case 'm':
//		g_snprintf (res, PATH_MAX, "%s", enc_track -> mp3_filename);
//		*munge = FALSE;
//		break;

	case 't':
		g_snprintf (res, PATH_MAX, "%02d", enc_track -> track_num + 1);
		*munge = FALSE;
		break;

	case 's':
		g_snprintf (res, PATH_MAX, "%d", enc_track -> ginfo -> start_sector);
		*munge = FALSE;
		break;

	case 'e':
		g_snprintf (res, PATH_MAX, "%d", enc_track -> ginfo -> end_sector);
		*munge = FALSE;
		break;

	case 'n':
		if (* (enc_track -> song_name)) {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> song_name);
		} else {
			g_snprintf (res, PATH_MAX, "Track%02d", enc_track -> track_num + 1);
		}

		break;

	case 'a':
		if (* (enc_track -> song_artist)) {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> song_artist);
		} else {
			if (* (enc_track -> disc_artist)) {
				g_snprintf (res, PATH_MAX, "%s", enc_track -> disc_artist);
			} else {
				strncpy (res, _("NoArtist"), PATH_MAX);
			}
		}

		break;

	case 'A':
		if (* (enc_track -> disc_artist)) {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> disc_artist);
		} else {
			strncpy (res, _("NoArtist"), PATH_MAX);
		}

		break;

	case 'd':
		if (* (enc_track -> disc_name)) {
			g_snprintf (res, PATH_MAX, "%s", enc_track -> disc_name);
		} else {
			strncpy (res, _("NoTitle"), PATH_MAX);
		}

		break;

	case 'i':
		g_snprintf (res, PATH_MAX, "%08x", enc_track -> discid);
		*munge = FALSE;
		break;

	case 'y':
		g_snprintf (res, PATH_MAX, "%d", enc_track -> song_year);
		*munge = FALSE;
		break;

	case 'G':
		g_snprintf (res, PATH_MAX, "%s", enc_track -> genre);
		*munge = FALSE;
		break;

	case 'r':
		g_snprintf (res, PATH_MAX, "%+6.2f", enc_track -> track_gain_adjustment);
		*munge = FALSE;
		break;

	case 'R':
		g_snprintf (res, PATH_MAX, "%+6.2f", enc_track -> disc_gain_adjustment);
		*munge = FALSE;
		break;

	case 'x':
		g_snprintf (res, PATH_MAX, "%s", enc_track -> ginfo -> format -> extension);
		*munge = FALSE;
		break;

	default:
		*res = '\0';
		break;
	}

	return res;
}

static void CheckDupNames (GripInfo *ginfo) {
	int track, track2;
	int numdups[MAX_TRACKS];
	int count;
	char buf[MAX_STRING];

	for (track = 0; track < ginfo -> disc.num_tracks; track++) {
		numdups[track] = 0;
	}

	for (track = 0; track < (ginfo -> disc.num_tracks - 1); track++) {
		if (!numdups[track]) {
			count = 0;

			for (track2 = track + 1; track2 < ginfo -> disc.num_tracks; track2++) {
				if (!strcmp (ginfo -> ddata.data_track[track].track_name,
							 ginfo -> ddata.data_track[track2].track_name)) {
					numdups[track2] = ++count;
				}
			}
		}
	}

	for (track = 0; track < ginfo -> disc.num_tracks; track++) {
		if (numdups[track]) {
			// FIXME: This is not working if track_name is already 256 chars long
			g_snprintf (buf, MAX_STRING, "%s (%d)", ginfo -> ddata.data_track[track].track_name,
						numdups[track] + 1);

			strcpy (ginfo -> ddata.data_track[track].track_name, buf);
		}
	}
}

void DoRip (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo;

	ginfo = (GripInfo *) data;

	/* Already ripping? */
	if (ginfo -> ripping) {
		return;
	}

//	if (!ginfo -> have_disc) {
//		show_warning (ginfo -> gui_info.app,
//					  _("No disc was detected in the drive. If you have a disc in your drive, please check your CD-Rom device setting under Config -> CD."));
//		return;
//	}

	CDStop (&(ginfo -> disc));
	ginfo -> stopped = TRUE;

	/* Close the device so as not to conflict with ripping */
	CDCloseDevice (& (ginfo -> disc));

	/* Initialize gain calculation */
	if (ginfo -> calc_gain) {
		InitGainAnalysis (44100);
	}

	CheckDupNames (ginfo);

	if (ginfo -> rip_partial) {
		ginfo -> rip_track = CURRENT_TRACK;
	} else {
		if (ginfo -> add_m3u) {
			AddM3U (ginfo);
		}

		SetCurrentTrackIndex (ginfo, 0);
		ginfo -> rip_track = 0;
	}

	if (NextTrackToRip (ginfo) == ginfo -> disc.num_tracks) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (ginfo -> gui_info.app),
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
							_("No tracks selected.\nRip whole CD?\n"));
//	gtk_window_set_title (GTK_WINDOW (dialog), "Warning");
		g_signal_connect (dialog,
						  "response",
						  G_CALLBACK (RipWholeCD),
						  ginfo);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	ginfo -> stop_rip = FALSE;

	CalculateAll (ginfo);

	RipNextTrack (ginfo);
}

static void RipWholeCD (GtkDialog *dialog, gint reply, gpointer data) {
	int track;
	GripInfo *ginfo;

	if (reply != GTK_RESPONSE_YES) {
		return;
	}

	g_debug ("Ripping whole CD");

	ginfo = (GripInfo *) data;

	for (track = 0; track < ginfo -> disc.num_tracks; ++track) {
		SetChecked (& (ginfo -> gui_info), track, TRUE);
	}

	DoRip (NULL, ginfo);
}

static int NextTrackToRip (GripInfo *ginfo) {
	int track;

	for (track = 0; (track < ginfo -> disc.num_tracks) &&
			(!TrackIsChecked (& (ginfo -> gui_info), track) ||
			 ! (ginfo -> disc).track[track].is_audio); track++)
		;

	return track;
}

/* Do the replay gain calculation on a sector */
static void gain_calc (gint16 *buffer, gsize bufsize) {
	static Float_t l_samples[588];
	static Float_t r_samples[588];
	long count;

	g_assert (bufsize == 588 * 2 * 2);

	for (count = 0; count < 588; count++) {
		l_samples[count] = (Float_t) buffer[count * 2];
		r_samples[count] = (Float_t) buffer[(count * 2) + 1];
	}

	AnalyzeSamples (l_samples, r_samples, 588, 2);
}

static gboolean rip_callback (gint16 *buffer, gsize bufsize, gfloat progress, int smilie_idx, gpointer user_data) {
	GripInfo *ginfo = (GripInfo *) user_data;

	ginfo -> rip_percent = progress;
	// Don't update GUI here, since this function is called from another thread!

	if (ginfo -> calc_gain)
		gain_calc (buffer, bufsize);

//    g_debug ("%s", get_smilie (smilie_idx));

	// We shall return FALSE to abort the rip, TRUE to continue
	g_assert (ginfo -> encoder != NULL);
	GError *error = NULL;
	if (!ginfo -> encoder -> callback (ginfo -> encoder -> handle, ginfo -> encoder_data, buffer, bufsize, &error) && ! (ginfo -> stop_rip)) {
		if (error) {
			g_warning ("Encoder callback failed: %s", error -> message);
			g_error_free (error);
		} else {
			g_warning ("Encoder callback failed");
		}

		return FALSE;
	}

	return TRUE;
}

static gboolean RipNextTrack (GripInfo *ginfo) {
	GripGUI *uinfo;
	char tmp[MAX_STRING];
	unsigned long long bytesleft;
	GString *str;
	EncodeTrack enc_track;
	char *ripfile, *ripfilefull;
	GError *error;

	uinfo = & (ginfo -> gui_info);

	g_debug ("In RipNextTrack");

	if (ginfo -> ripping) {
		return FALSE;
	}

	if (!ginfo -> rip_partial) {
		ginfo -> rip_track = NextTrackToRip (ginfo);
		g_assert (ginfo -> rip_track <= ginfo -> disc.num_tracks);
	}
	g_debug ("First checked track is %d", ginfo -> rip_track + 1);

	/* See if we are finished ripping */
	if (ginfo -> rip_track == ginfo -> disc.num_tracks) {
		return FALSE;
	}

	/* We have a track to rip */
	if (ginfo -> have_disc && ginfo -> rip_track >= 0) {
		g_debug ("Ripping away!");

        GtkLabel *l = GTK_LABEL (gtk_builder_get_object (uinfo -> builder, "label_ripping"));
        gchar *txt = g_strdup_printf (_("Ripping Track %d"), ginfo -> rip_track + 1);
        gtk_label_set (l, txt);
        g_free (txt);

		/*    if(!ginfo -> rip_partial){
		  gtk_clist_select_row(GTK_CLIST(uinfo -> trackclist),ginfo -> rip_track,0);
		  }*/

		CopyPixmap (GTK_PIXMAP (uinfo -> rip_pix[0]), GTK_PIXMAP (uinfo -> rip_indicator));

		if (ginfo -> stop_between_tracks) {
			CDStop (& (ginfo -> disc));
		}

		if (!ginfo -> rip_partial) {
			ginfo -> start_sector = 0;
			ginfo -> end_sector = (ginfo -> disc.track[ginfo -> rip_track + 1].start_frame - 1) -
								  ginfo -> disc.track[ginfo -> rip_track].start_frame;

			/* Compensate for the gap before a data track */
			if ((ginfo -> rip_track < (ginfo -> disc.num_tracks - 1) &&
					!(ginfo -> disc).track[ginfo -> rip_track + 1].is_audio &&
					(ginfo -> end_sector - ginfo -> start_sector) > 11399)) {
				ginfo -> end_sector -= 11400;
			}
		}

		ginfo -> ripsize = 44 + ((ginfo -> end_sector - ginfo -> start_sector) + 1) * 2352;

		// Generate output filename
		str = g_string_new (NULL);
		FillInTrackInfo (ginfo, ginfo -> rip_track, &enc_track);
		TranslateString (ginfo -> ripfileformat, str, TranslateSwitch,
						 &enc_track, TRUE, & (ginfo -> sprefs));

		ripfile = g_filename_from_utf8 (str -> str, strlen (str -> str), NULL, NULL, &error);
		if (!ripfile) {
			g_warning ("Error while converting UTF-8 to filename: %s", error -> message);
			g_error_free (error);
			ripfile = g_strdup (str -> str);
		}
		g_string_free (str, TRUE);

		// Make ripfile relative to output folder (which should already be in filename encoding)
		ripfilefull = g_build_filename (ginfo -> output_folder, ripfile, NULL);
		g_free (ripfile);
		ripfile = NULL;
		g_snprintf (ginfo -> ripfile, PATH_MAX, "%s", ripfilefull);
		g_free (ripfilefull);

		// Check for writability. Note that dir can be different from ginfo -> output_folder,
		// since directories can also be specified in ginfo -> ripfileformat.
		gchar *dir = g_path_get_dirname (ginfo -> ripfile);
		if (dir == NULL || g_mkdir_with_parents (dir, 0755) < 0 || g_access (dir, W_OK) != 0) {
			show_error (ginfo -> gui_info.app,
						_("No write access to write output file"));
			g_free (dir);
			return FALSE;
		}

		// Check if file already exists
		GStatBuf mystat;
		if (g_stat (ginfo -> ripfile, &mystat) >= 0) {
			if (mystat.st_size > 0) {
				g_debug ("File %s has already been ripped. Skipping...", ginfo -> ripfile);

//				ginfo -> ripping = TRUE;
//				ginfo -> all_ripdone += CalculateWavSize (ginfo, ginfo -> rip_track);
//				ginfo -> all_riplast = 0;

				// FIXME: Free stuff?
				return TRUE;
			} else {
				g_unlink (ginfo -> ripfile);
			}
		}

		// This is currently wrong, but works as a sort of (much higher) upper limit, so let's keep it
		bytesleft = BytesLeftInFS (ginfo -> ripfile);
		if (bytesleft < (ginfo -> ripsize * 1.5)) {
			show_error (ginfo -> gui_info.app,
						_("Out of space in output directory"));
			// FIXME: Free stuff?
			return FALSE;
		}

		// Now that we are sure we can write to output file, generate a temporary one
		// Reuse ripfilefull
		ripfilefull = g_build_filename (dir, "regrip-XXXXXX.tmp", NULL);
		g_snprintf (ginfo -> rip_tmpfile, PATH_MAX, "%s", ripfilefull);
		g_free (ripfilefull);
		if ((ginfo -> riptmpfd = g_mkstemp (ginfo -> rip_tmpfile)) < 0) {
			show_error (ginfo -> gui_info.app,
						_("Cannot create temporary rip file"));
			// FIXME: Free stuff?
			return FALSE;
		}


		g_message ("Ripping track %d to '%s'", ginfo -> rip_track + 1, ginfo -> ripfile);
		g_debug ("Temp file is: '%s'", ginfo -> rip_tmpfile);

		// Init encoder
		g_assert (ginfo -> encoder != NULL);
		FILE *fp = fdopen (ginfo -> riptmpfd, "wb");
		g_assert (fp);
		error = NULL;
		gpointer encoder = ginfo -> encoder -> start (ginfo -> encoder -> handle, ginfo -> format, fp, &error);
		if (!encoder) {
			gchar *warn = g_strdup_printf (_("Cannot start encoder: %s"), error -> message);
			show_error (ginfo -> gui_info.app, warn);
			g_free (warn);
			g_error_free (error);
			return FALSE;
		}

		// This is no longer needed
		g_free (dir);

		// Start rip thread
		ginfo -> rip_started = time (NULL);
		ginfo -> rip_percent = 0;
		ginfo -> encoder_data = encoder;
		g_snprintf (tmp, MAX_STRING, _("Rip: Trk %d (0.0x)"), ginfo -> rip_track + 1);
//		gtk_label_set (GTK_LABEL (uinfo -> rip_prog_label), tmp);

		error = NULL;
		if (!rip_start (ginfo, rip_callback, ginfo, &error)) {
			gchar *warn = g_strdup_printf (_("Cannot start rip thread: %s"), error -> message);
			show_error (ginfo -> gui_info.app, warn);
			g_free (warn);
			g_error_free (error);
			return FALSE;
		}

		ginfo -> ripping = TRUE;

		return TRUE;
	} else {
		return FALSE;
	}
}

void FillInTrackInfo (GripInfo *ginfo, int track, EncodeTrack *new_track) {
	new_track -> ginfo = ginfo;

	new_track -> wav_filename[0] = '\0';
//	new_track -> mp3_filename[0] = '\0';

	new_track -> track_num = track;
	new_track -> start_frame = ginfo -> disc.track[track].start_frame;
	new_track -> end_frame = ginfo -> disc.track[track + 1].start_frame - 1;
	new_track -> track_gain_adjustment = ginfo -> track_gain_adjustment;
	new_track -> disc_gain_adjustment = ginfo -> disc_gain_adjustment;

	/* Compensate for the gap before a data track */
	if ((track < (ginfo -> disc.num_tracks - 1) &&
			!(ginfo -> disc).track[track + 1].is_audio &&
			(new_track -> end_frame - new_track -> start_frame) > 11399)) {
		new_track -> end_frame -= 11400;
	}

	new_track -> mins = ginfo -> disc.track[track].length.mins;
	new_track -> secs = ginfo -> disc.track[track].length.secs;
	new_track -> song_year = ginfo -> ddata.data_year;
	g_snprintf (new_track -> song_name, MAX_STRING, "%s",
				ginfo -> ddata.data_track[track].track_name);
	g_snprintf (new_track -> song_artist, MAX_STRING, "%s",
				ginfo -> ddata.data_track[track].track_artist);
	g_snprintf (new_track -> disc_name, MAX_STRING, "%s", ginfo -> ddata.data_title);
	g_snprintf (new_track -> disc_artist, MAX_STRING, "%s", ginfo -> ddata.data_artist);
	strcpy (new_track -> genre, ginfo -> ddata.data_genre);
	new_track -> discid = ginfo -> ddata.data_id;
}

#if 0
static void AddToEncode (GripInfo *ginfo, int track) {
	EncodeTrack *new_track;

	new_track = (EncodeTrack *) g_new (EncodeTrack, 1);

	FillInTrackInfo (ginfo, track, new_track);
	strcpy (new_track -> wav_filename, ginfo -> ripfile);

	if (!ginfo -> delayed_encoding) {
		ginfo -> encode_list = g_list_append (ginfo -> encode_list, new_track);
	} else {
		ginfo -> pending_list = g_list_append (ginfo -> pending_list, new_track);
	}

	g_debug (_("Added track %d to %s list"), track + 1,
			 ginfo -> delayed_encoding ? "pending" : "encoding");
}

static gboolean MP3Encode (GripInfo *ginfo) {
	GripGUI *uinfo;
	char tmp[PATH_MAX];
	int arg;
	GString *args[100];
	char *char_args[101];
	unsigned long long bytesleft;
	EncodeTrack *enc_track;
	GString *str;
	int encode_track;
	int cpu;
	char *conv_str;
	gsize rb, wb;

	uinfo = & (ginfo -> gui_info);

	if (!ginfo -> encode_list) {
		return FALSE;
	}

	for (cpu = 0; (cpu < ginfo -> num_cpu) && (ginfo -> encoding & (1 << cpu)); cpu++);

	if (cpu == ginfo -> num_cpu) {
		g_debug (_("No free cpus"));
		return FALSE;
	}

	enc_track = (EncodeTrack *) (g_list_first (ginfo -> encode_list) -> data);
	encode_track = enc_track -> track_num;

	ginfo -> encode_list = g_list_remove (ginfo -> encode_list, enc_track);
	ginfo -> encoded_track[cpu] = enc_track;

	CopyPixmap (GTK_PIXMAP (uinfo -> mp3_pix[0]),
				GTK_PIXMAP (uinfo -> mp3_indicator[cpu]));

	ginfo -> mp3_started[cpu] = time (NULL);
	ginfo -> mp3_enc_track[cpu] = encode_track;

	g_debug (_("Enc track %d"), encode_track + 1);

	strcpy (ginfo -> rip_delete_file[cpu], enc_track -> wav_filename);

	str = g_string_new (NULL);

	TranslateString (ginfo -> mp3fileformat, str, TranslateSwitch,
					 enc_track, TRUE, & (ginfo -> sprefs));

	conv_str = g_filename_from_utf8 (str -> str, strlen (str -> str), &rb, &wb, NULL);

	if (!conv_str) {
		conv_str = g_strdup (str -> str);
	}

	g_snprintf (ginfo -> mp3file[cpu], MAX_STRING, "%s", conv_str);

	g_free (conv_str);
	g_string_free (str, TRUE);

	MakeDirs (ginfo -> mp3file[cpu]);

	if (!CanWrite (ginfo -> mp3file[cpu])) {
		show_warning (ginfo -> gui_info.app,
					  _("No write access to write encoded file."));
		return FALSE;
	}

	bytesleft = BytesLeftInFS (ginfo -> mp3file[cpu]);

	conv_str = g_filename_to_utf8 (ginfo -> mp3file[cpu],
								   strlen (ginfo -> mp3file[cpu]),
								   &rb, &wb, NULL);

	if (!conv_str) {
		conv_str = strdup (ginfo -> mp3file[cpu]);
	}

	g_message (ginfo, _("%i: Encoding to %s\n"), cpu + 1, conv_str);

	g_free (conv_str);

	sprintf (tmp, _("Enc: Trk %d (0.0x)"), encode_track + 1);
	gtk_label_set (GTK_LABEL (uinfo -> mp3_prog_label[cpu]), tmp);

	unlink (ginfo -> mp3file[cpu]);

	ginfo -> mp3size[cpu] =
		(int) ((gfloat) ((enc_track -> end_frame - enc_track -> start_frame) + 1) *
				(gfloat) (ginfo -> kbits_per_sec * 1024) / 600.0);

	if (bytesleft < (ginfo -> mp3size[cpu] * 1.5)) {
		show_warning (ginfo -> gui_info.app,
					  _("Out of space in output directory"));

		return FALSE;
	}

	strcpy (enc_track -> mp3_filename, ginfo -> mp3file[cpu]);

	MakeTranslatedArgs (ginfo -> mp3cmdline, args, 100, TranslateSwitch,
						enc_track, FALSE, & (ginfo -> sprefs));

	/*
	  ArgsToLocale(args);
	*/

	for (arg = 0; args[arg]; arg++) {
		char_args[arg + 1] = args[arg] -> str;
	}

	char_args[arg + 1] = NULL;

	char_args[0] = FindRoot (ginfo -> mp3exename);

	ginfo -> curr_pipe_fd =
		GetStatusWindowPipe (ginfo -> gui_info.encode_status_window);

	ginfo -> mp3pid[cpu] = fork();

	if (ginfo -> mp3pid[cpu] == 0) {
		CloseStuff (ginfo);
		setsid();
		nice (ginfo -> mp3nice);
		execv (ginfo -> mp3exename, char_args);
		_exit (0);
	} else {
		ginfo -> curr_pipe_fd = -1;
	}

	for (arg = 0; args[arg]; arg++) {
		g_string_free (args[arg], TRUE);
	}

	ginfo -> encoding |= (1 << cpu);

	return TRUE;
}
#endif

void CalculateAll (GripInfo *ginfo) {
	int track;
	GripGUI *uinfo;

	uinfo = & (ginfo -> gui_info);

	g_debug (_("In CalculateAll"));

	ginfo -> all_ripsize = 0;
	ginfo -> all_ripdone = 0;
	ginfo -> all_riplast = 0;

//	if (!ginfo -> encoding) {
//		g_debug (_("We aren't ripping now, so let's zero encoding values"));
//		ginfo -> all_encsize = 0;
//		ginfo -> all_encdone = 0;
//
//        ginfo -> all_enclast = 0;
//	}

	if (ginfo -> rip_partial) {
		return;
	}

	for (track = 0; track < ginfo -> disc.num_tracks; ++track) {
		if ((ginfo -> disc).track[track].is_audio &&
				(TrackIsChecked (uinfo, track)) ) {
			ginfo -> all_ripsize += CalculateWavSize (ginfo, track);
			ginfo -> all_encsize += CalculateEncSize (ginfo, track);
		}
	}

	g_debug (_("Total rip size is: %zu"), ginfo -> all_ripsize);
	g_debug (_("Total enc size is: %zu"), ginfo -> all_encsize);
}

static size_t CalculateWavSize (GripInfo *ginfo, int track) {
	int frames;

	frames = (ginfo -> disc.track[track + 1].start_frame - 1) -
			 ginfo -> disc.track[track].start_frame;

	if (track < (ginfo -> disc.num_tracks) - 1 &&
			! (ginfo -> disc).track[track + 1].is_audio &&
			frames > 11399) {
		frames -= 11400;
	}

	return frames * 2352;
}

static size_t CalculateEncSize (GripInfo *ginfo, int track) {
	double tmp_encsize;
	/* It's not the best way, but i couldn't find anything better */
	tmp_encsize = (double) ((ginfo -> disc.track[track].length.mins * 60 +
							  ginfo -> disc.track[track].length.secs - 2));
	tmp_encsize -= tmp_encsize * 0.0154;

	if (ginfo -> add_m3u) {
		tmp_encsize += 128;
	}

	return (size_t) tmp_encsize;
}


/********************** PARTIAL RIP **********************/

static void on_rip_partial_sector_changed (GtkSpinButton *spinbutton, gpointer user_data) {
	*((int *) user_data) = gtk_spin_button_get_value_as_int (spinbutton);
}


#define STOCK_BTN_PLAY "gtk-media-play"
#define STOCK_BTN_STOP "gtk-media-stop"

static void PlaySegmentCB (GtkWidget *widget, gpointer data) {
	GripInfo *ginfo = (GripInfo *) data;

	// Toggle button between Play/Stop
	if (strcmp (gtk_button_get_label (GTK_BUTTON (widget)), STOCK_BTN_PLAY) == 0) {
        gtk_button_set_label (GTK_BUTTON (widget), STOCK_BTN_STOP);
        PlaySegment (ginfo, CURRENT_TRACK);
    } else {
        gtk_button_set_label (GTK_BUTTON (widget), STOCK_BTN_PLAY);
        CDStop (&(ginfo -> disc));
    }
}

// Gets called when the "Partial Rip..." menu option is selected
void on_menuitem_rip_partial_activate (GtkMenuItem *menuitem, gpointer user_data) {
	GripInfo *ginfo = (GripInfo *) user_data;
	GripGUI *uinfo = &(ginfo -> gui_info);

	GtkDialog *dialog = GTK_DIALOG (gtk_builder_get_object (uinfo -> builder, "dialog_partial_rip"));
	g_assert (dialog);

    uinfo -> play_sector_label = GTK_WIDGET (gtk_builder_get_object (uinfo -> builder, "entry_rip_partial_current"));
    g_assert (uinfo -> play_sector_label);

	// Useful aliases for setting SpinButton ranges
	DiscInfo *disc = &(ginfo -> disc);
	TrackInfo *first_track = &(disc -> track[0]);
	TrackInfo *last_track = &(disc -> track[disc -> num_tracks - 1]);
	gdouble first_sector = first_track -> start_frame;
	gdouble last_sector = last_track -> start_frame + (last_track -> length).frames - 1;

	GtkSpinButton *spin_start = GTK_SPIN_BUTTON (gtk_builder_get_object (uinfo -> builder, "spinbutton_rip_partial_start"));
	g_assert (spin_start);
	gtk_spin_button_set_increments (spin_start, 1, 75);
	g_signal_connect (spin_start, "value-changed", G_CALLBACK (on_rip_partial_sector_changed), &(ginfo -> start_sector));
	gtk_spin_button_set_range (spin_start, first_sector, last_sector);      	// Set range after handler is connected

	GtkSpinButton *spin_end = GTK_SPIN_BUTTON (gtk_builder_get_object (uinfo -> builder, "spinbutton_rip_partial_end"));
	g_assert (spin_end);
	gtk_spin_button_set_increments (spin_end, 1, 75);
	g_signal_connect (spin_end, "value-changed", G_CALLBACK (on_rip_partial_sector_changed), &(ginfo -> end_sector));
    gtk_spin_button_set_range (spin_end, first_sector, last_sector);
	gtk_spin_button_set_value (spin_end, last_sector);

	GtkButton *button_partial_rip_play = GTK_BUTTON (gtk_builder_get_object (uinfo -> builder, "button_partial_rip_play"));
	g_assert (button_partial_rip_play);
	g_signal_connect (button_partial_rip_play, "clicked", G_CALLBACK (PlaySegmentCB), ginfo);

	gtk_dialog_run (dialog);
	gtk_widget_hide (GTK_WIDGET (dialog));
}
