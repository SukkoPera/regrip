/* grip.h
 *
 * Copyright (c) 1998-2002  Mike Oliphant <oliphant@gtk.org>
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
 *
 */

#ifndef GRIP_H
#define GRIP_H

#include "config.h"
#include "cddev.h"
#include "discdb.h"
#include "pthread.h"
#include "launch.h"
#include "status_window.h"
#include "eggtrayicon.h"
#include "encoder.h"
#include "common.h"

#define RRand(range) (random () % (range))

#define UI_FILE "/home/sukko/Development/grip/ui/regrip.ui"


typedef struct _grip_gui {
    GtkBuilder *builder;

	GtkWidget *app;
	GtkWidget *winbox;
	GtkWidget *notebook;
	GtkStatusbar *statusbar;

	GtkWidget *preferences;

	gboolean minimized;
	int win_width;
	int win_height;
	int win_height_edit;
	int win_width_min;
	int win_height_min;
	int x;
	int y;
	GtkStyle *style_wb;
	GtkStyle *style_LCD;
	GtkStyle *style_dark_grey;

	GtkWidget *disc_name_label;
	GtkWidget *disc_artist_label;
	GtkListStore *track_list_store;
	GtkWidget *track_list;

	GtkWidget *current_track_label;
	int time_display_mode;
	GtkWidget *play_time_label;
	GtkWidget *rip_indicator;
	GtkWidget *lcd_smile_indicator;
	GtkWidget *mp3_indicator;
	GtkWidget *discdb_indicator;
	GtkWidget *control_button_box;
	GtkWidget *controls;
	gboolean control_buttons_visible;
	GdkCursor *wait_cursor;

	gboolean track_edit_visible;
	GtkWidget *track_edit_box;
	GtkWidget *artist_edit_entry;
	GtkWidget *title_edit_entry;
	GtkWidget *genre_combo;
	GtkWidget *year_spin_button;
	GtkWidget *track_edit_entry;
	GtkWidget *multi_artist_box;
	GtkWidget *track_artist_edit_entry;
	GtkWidget *split_chars_entry;
	GtkWidget *multi_artist_button;
	GtkWidget *playopts;
	GtkWidget *playlist_entry;
	GtkWidget *play_indicator;
	GtkWidget *loop_indicator;
	gboolean track_prog_visible;

	GtkWidget *volume_control;
	gboolean volvis;

	StatusWindow *status_window;
	StatusWindow *rip_status_window;
	StatusWindow *encode_status_window;

	GtkWidget *play_sector_label;

	GtkWidget *partial_rip_box;
	GtkWidget *rip_prog_label;
	GtkWidget *ripprogbar;
	GtkWidget *smile_indicator;

	GtkWidget *start_sector_entry;
	GtkWidget *end_sector_entry;

	/* Overall progress */
	GtkWidget *all_label;
	GtkWidget *all_rip_label;
//	GtkWidget *all_enc_label;
	GtkWidget *all_ripprogbar;
//	GtkWidget *all_encprogbar;

    /* DiscDB */
    GtkWidget *discdb_server;

	/* Proxy */
	GtkWidget *proxy_name;
	GtkWidget *proxy_port;
	GtkWidget *proxy_user;
	GtkWidget *proxy_pswd;
	GtkWidget *proxy_use_env;
	GtkWidget *proxy_use;

	/* Images */
	GtkWidget *check_image;
	GtkWidget *eject_image;
	GtkWidget *cdscan_image;
	GtkWidget *ff_image;
	GtkWidget *lowleft_image;
	GtkWidget *lowright_image;
	GtkWidget *minmax_image;
	GtkWidget *nexttrk_image;
	GtkWidget *playpaus_image;
	GtkWidget *prevtrk_image;
	GtkWidget *loop_image;
	GtkWidget *noloop_image;
	GtkWidget *random_image;
	GtkWidget *playlist_image;
	GtkWidget *playnorm_image;
	GtkWidget *quit_image;
	GtkWidget *rew_image;
	GtkWidget *stop_image;
	GtkWidget *upleft_image;
	GtkWidget *upright_image;
	GtkWidget *vol_image;
	GtkWidget *discdbwht_image;
	GtkWidget *rotate_image;
	GtkWidget *edit_image;
	GtkWidget *progtrack_image;
	GtkWidget *mail_image;
	GtkWidget *save_image;
	GtkWidget *empty_image;

	GtkWidget *discdb_pix[2];
	GtkWidget *rip_pix[4];
	GtkWidget *mp3_pix[4];
	GtkWidget *smile_pix[8];

	GtkWidget *play_pix[3];

	/* notification area widgets */
	EggTrayIcon *tray_icon;
	GtkTooltips *tray_tips;
	GtkWidget *tray_ebox;
	GtkWidget *tray_menu;
	GtkWidget *tray_menu_play;
	GtkWidget *tray_menu_pause;
} GripGUI;

struct _encode_track;

/* HTTP proxy server structure */
typedef struct _proxy_server {
	char name[MAX_STRING];
	int port;
	char username[MAX_STRING];
	char pswd[MAX_STRING];
} ProxyServer;

typedef enum {
    DISCDB_IDLE,
    DISCDB_QUERYING,
    DISCDB_RESULTS_READY
} discdb_thread_status;

typedef struct _grip_info {
    /* Program version */
	char version[MAX_STRING];

	/* GUI info */
    GripGUI gui_info;

	/* Physical disc information (i.e. offsets and such) */
    char cd_device[MAX_STRING];
	char force_scsi[MAX_STRING];
	DiscInfo disc;

	/* Disc Data (i.e.: Album/Song titles/artist, etc.) */
	DiscData ddata;

	/* Network connection */
	gboolean use_proxy;
	gboolean use_proxy_env;
	ProxyServer proxy_server;
	gboolean local_mode;

	/* DiscDB stuff */
	DiscDBServer dbserver;
	GList *cddb_results;
	GThread *discdb_thread;
	char discdb_submit_email[MAX_STRING];
	char discdb_encoding[SMALL_STRING];
//	char user_email[MAX_STRING];
	discdb_thread_status looking_up;

	char id3_encoding[SMALL_STRING];
//	char id3v2_encoding[SMALL_STRING];
	gboolean use_freedb;
	gboolean update_required;
	gboolean have_disc;
	gboolean tray_open;
	gboolean faulty_eject;
	gboolean ask_submit;
	gboolean is_new_disc;
	gboolean first_time;
	gboolean play_first;
	gboolean play_on_insert;
	gboolean stop_first;
	gboolean no_interrupt;
	gboolean automatic_discdb;
	gboolean poll_drive;
	int poll_interval;
	int auto_eject_countdown;
	int volume;

	int current_disc;

	gboolean playing;
	gboolean stopped;
	gboolean ffwding;
	gboolean rewinding;
	int play_mode;
	gboolean playloop;
	int current_track_index;
	int tracks_prog[MAX_TRACKS];
	int prog_totaltracks;
	gboolean automatic_reshuffle;

	char title_split_chars[6];

	int curr_pipe_fd;

	gboolean ripping;
	gboolean stop_rip;
	time_t rip_finished;
	int rip_track;
	time_t rip_started;
	int ripsize;
	char ripfile[PATH_MAX];
	int riptmpfd;
	int start_sector;
	int end_sector;

	double track_gain_adjustment;
	double disc_gain_adjustment;
	struct _encode_track *encoded_track;
	gboolean do_redirect;

	gpointer encoder_data;

	/* Ripper */
	gboolean in_rip_thread;
	GThread *rip_thread;
	gboolean disable_paranoia;
	gboolean disable_extra_paranoia;
	gboolean disable_scratch_detect;
	gboolean disable_scratch_repair;
	int rip_smile_level;
	gfloat rip_percent_done;

	gboolean calc_gain;

	char output_folder[PATH_MAX];
	char rip_tmpfile[PATH_MAX];
	char ripfileformat[MAX_STRING];
//	int max_wavs;
	gboolean auto_rip;
	gboolean beep_after_rip;
	gboolean eject_after_rip;
	gboolean rip_partial;
	int eject_delay;
	gboolean delay_before_rip;
	gboolean stop_between_tracks;
	char wav_filter_cmd[MAX_STRING];
	char disc_filter_cmd[MAX_STRING];
//	int selected_encoder;
	supported_encoder *encoder;
	supported_format *format;
//	gboolean delete_wavs;
	gboolean add_m3u;
	gboolean rel_m3u;
	char m3ufileformat[MAX_STRING];
//	int kbits_per_sec;
//	int edit_num_cpu;
//	int mp3nice;
	char mp3_filter_cmd[MAX_STRING];
	gboolean doid3;
//	gboolean doid3v2;
//	gboolean tag_mp3_only;
	char id3_comment[MAX_STRING];
	char cdupdate[MAX_STRING];
	StrTransPrefs sprefs;
	gboolean keep_min_size;

	/* some vars for use in TrayIconUpdate */
	gfloat rip_percent;
//	gfloat enc_percent;

	gfloat rip_tot_percent;
//	gfloat enc_tot_percent;

	gboolean app_visible;

	gboolean show_tray_icon;
	gboolean tray_icon_made;
	gboolean tray_menu_sensitive;

	/* these are for calculating ripping progress */
	size_t all_ripsize;
	size_t all_ripdone;
	size_t all_riplast;
	size_t all_encsize;     // Estimated size of ripped and compressed tracks (Really necessary?)
//	size_t all_encdone;
//	size_t all_enclast;

	// App settings
	GSettings *settings;
	GSettings *settings_cdplay;
	GSettings *settings_cdparanoia;
	GSettings *settings_rip;
	GSettings *settings_encoder;
	GSettings *settings_tag;
	GSettings *settings_discdb;
	GSettings *settings_proxy;
} GripInfo;

GtkWidget *GripNew (const gchar* geometry, char *device, char *scsi_device,
					char *config_filename, gboolean force_small,
					gboolean local_mode, gboolean no_redirect);
void GripDie (GtkWidget *widget, gpointer data);
void GripUpdate (GtkWidget *app);
GtkWidget *MakeNewPage (GtkWidget *notebook, char *name);
void Busy (GripGUI *uinfo);
void UnBusy (GripGUI *uinfo);
void CloseStuff (void *user_data);

#endif /* ifndef GRIP_H */
