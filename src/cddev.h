/* cddev.h
 *
 * Based on code from libcdaudio 0.5.0 (Copyright (C)1998 Tony Arcieri)
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
 *
 */

#ifndef GRIP_CDDEV_H
#define GRIP_CDDEV_H

#include <glib.h>
//#include <cdio/cdio.h>
//#include <cdio/cdio_unconfig.h>
#include "common.h"

/* Used with disc_info */
#define CDAUDIO_PLAYING             0
#define CDAUDIO_PAUSED              1
#define CDAUDIO_COMPLETED           2
#define CDAUDIO_NOSTATUS            3

#define MAX_SLOTS               100 /* For CD changers */


typedef struct {
    gchar device[MAX_STRING];
    gchar vendor[MAX_STRING];
    gchar model[MAX_STRING];
    gchar revision[MAX_STRING];
} cd_drive;

/* Used for keeping track of times */
typedef struct _disc_time {
	int mins;
	int secs;

	/* These are total frames (i.e.: the equivalent of the above), *not* the
	 * frames part of mins:secs:frames!
	 */
	long frames;
} DiscTime;

/* Track specific information */
typedef struct _track_info {
    int num;                // Actual track number
	DiscTime length;
	DiscTime start_pos;
//	int num_frames;
	gint32 start_frame;
	gboolean is_audio;
//	unsigned char flags;
} TrackInfo;

// Forward declation from libcdio headers
typedef struct _CdIo CdIo_t;

/* Disc information such as current track, amount played, etc */
typedef struct _disc_info {
    CdIo_t *cdio;
	int cd_desc;                              /* CD device file desc. */
	char *devname;                            /* CD device file pathname */

	gboolean disc_present;                /* Is disc present? */
	int disc_mode;                    /* Current disc mode */
	DiscTime track_time;                  /* Current track time */
	DiscTime disc_time;                   /* Current disc time */
	DiscTime length;                  /* Total disc length */
//	int curr_frame;               /* Current frame */
	int curr_track;                   /* Current track */

	int first_track;
    int last_track;
    int first_audio_track;
    int last_audio_track;

	int num_tracks;                   /* Number of audio tracks on disc */
	int num_data_tracks;                   /* Number of data tracks on disc */
	TrackInfo track[MAX_TRACKS];              /* Track specific information */
} DiscInfo;

/* Channel volume structure */
typedef struct _channel_volume {
  int left;
  int right;
} ChannelVolume;

/* Volume structure */
typedef struct _disc_volume {
   ChannelVolume vol_front;
   ChannelVolume vol_back;
} DiscVolume;

GList *get_cd_drives (void);

gboolean CDInitDevice (char *device_name, DiscInfo *disc);
gboolean CDCloseDevice (DiscInfo *disc);
gboolean is_cd_inserted (DiscInfo *disc, const gchar **disc_type);
gboolean CDStat (DiscInfo *disc, gboolean force_read_toc);
gboolean CDPlayFrames (DiscInfo *disc, int startframe, int endframe);
gboolean CDPlayTrackPos (DiscInfo *disc, int starttrack,
                         int endtrack, int startpos);
gboolean CDPlayTrack (DiscInfo *disc, int starttrack, int endtrack);
gboolean CDAdvance (DiscInfo *disc, DiscTime *time);
gboolean CDStop (DiscInfo *disc);
gboolean CDPause (DiscInfo *disc);
gboolean CDResume (DiscInfo *disc);
gboolean IsTrayOpen (DiscInfo *disc);
gboolean CDEject (DiscInfo *disc);
gboolean CDClose (DiscInfo *disc);
gboolean CDGetVolume(DiscInfo *disc, DiscVolume *vol);
gboolean CDSetVolume(DiscInfo *disc, DiscVolume *vol);

#endif /* GRIP_CDDEV_H */
