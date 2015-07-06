/* cddev.c
 *
 * Based on code from libcdaudio 0.5.0 (Copyright (C)1998 Tony Arcieri)
 *
 * All changes copyright (c) 1998-2015 Mike Oliphant <contact@nostatic.org>
 * and 2014-2015 SukkoPera <software@sukkology.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <cdio/cd_types.h>
#include <cdio/util.h>
#include <cdio/audio.h>
#include <cdio/cdio_unconfig.h>          // remove *all* symbols libcdio defines
#include "cddev.h"
#include "uihelper.h"
#include <config.h>


GList *get_cd_drives (void) {
    GList *drives = NULL;

    g_debug ("Autodetecting CD-ROM drivers with a CD-DA loaded");

    char **drvs = cdio_get_devices_with_cap (NULL, CDIO_FS_AUDIO, false);
    char **c;
    for (c = drvs; c && *c != NULL; c++) {
        g_debug ("-- Found Drive %s", *c);

        CdIo_t *p_cdio = cdio_open (*c, DRIVER_DEVICE);
        if (p_cdio) {
            cdio_hwinfo_t hwinfo;

            if (cdio_get_hwinfo(p_cdio, &hwinfo)) {
                cd_drive *d = g_new (cd_drive, 1);
                strncpy (d -> device, *c, MAX_STRING);
                strncpy (d -> vendor, hwinfo.psz_vendor, MAX_STRING);
                strncpy (d -> model, hwinfo.psz_model, MAX_STRING);
                strncpy (d -> revision, hwinfo.psz_revision, MAX_STRING);
                drives = g_list_append (drives, d);
            } else {
                g_warning ("Unable to get drive hardware information for %s", *c);
            }

            cdio_destroy (p_cdio);
        }
    }

    cdio_free_device_list (drvs);

    g_debug ("Autodetection complete");

    return drives;
}

/* Initialize the CD-ROM for playing audio CDs */
gboolean CDInitDevice (char *device_name, DiscInfo *disc) {
//	memset (disc, 0x00, sizeof (DiscInfo)); // Just to make sure
//	disc -> toc_up_to_date = FALSE;
//	disc -> disc_present = FALSE;

	if (disc -> devname
	        && disc -> devname != device_name
	        && strcmp (device_name, disc -> devname)) {
		free (disc -> devname);
		disc -> devname = 0;
	}

	if (!disc -> devname) {
		disc -> devname = strdup (device_name);
	}

	disc -> cdio = cdio_open (device_name, DRIVER_DEVICE);
	if (!disc -> cdio) {
        g_warning (g_strerror (errno));
		return FALSE;
	}

	return TRUE;
}

gboolean CDCloseDevice (DiscInfo *disc) {
    if (disc -> devname) {
		free (disc -> devname);
	}

    if (disc -> cdio)
        cdio_destroy (disc -> cdio);
    disc -> cdio = NULL;

	return TRUE;
}

/* disc_type is an output parameter, only valid if TRUE is returned. Don't free() it! */
gboolean is_cd_inserted (DiscInfo *disc, const gchar **disc_type) {
    gboolean have_disc;

	if (!disc -> cdio) {
		CDInitDevice (disc -> devname, disc);
	}

    discmode_t discmode = cdio_get_discmode (disc -> cdio);
    if (discmode == CDIO_DISC_MODE_NO_INFO || discmode == CDIO_DISC_MODE_ERROR) {
        // Assume no disc inserted
        have_disc = FALSE;
//        g_debug ("No disc");
    } else {
        have_disc = TRUE;
        if (disc_type)
            *disc_type = discmode2str[discmode];
    }

    return have_disc;
}

/* Read TOC... God bless libcdio! */
gboolean CDStat (DiscInfo *disc, gboolean force_read_toc) {
    /* Read the Table Of Contents */
    g_debug ("Reading disc TOC");

    // libcdio caches the TOC, so we must reinit
    CDInitDevice (disc -> devname, disc);

    disc -> first_track = cdio_get_first_track_num (disc -> cdio);
    disc -> last_track = cdio_get_last_track_num (disc -> cdio);
    disc -> num_tracks  = cdio_get_num_tracks (disc -> cdio);
    disc -> first_audio_track = disc -> first_track;
    disc -> last_audio_track  = disc -> last_track;

    if (disc -> first_track == CDIO_INVALID_TRACK || disc -> last_track == CDIO_INVALID_TRACK) {
        g_error ("Cannot read TOC");
    } else {
        // Disc length (CDDB needs LBA, so use those)
        msf_t msf;          // Minutes/Seconds/Frames
        lba_t last_lba = cdio_lsn_to_lba (cdio_get_disc_last_lsn (disc -> cdio));
        disc -> length.frames = last_lba;
        cdio_lba_to_msf (last_lba, &msf);
        disc -> length.mins = cdio_from_bcd8 (msf.m);
        disc -> length.secs = cdio_from_bcd8 (msf.s);
        g_debug ("Disc len = %02d:%02d", disc -> length.mins, disc -> length.secs);

        disc -> num_data_tracks = 0;
        track_t i;
        int j = 0;
        for (i = disc -> first_track; i <= disc -> last_track; i++) {
            if (cdio_get_track_format (disc -> cdio, i) != TRACK_FORMAT_AUDIO) {
                ++(disc -> num_data_tracks);
                if (i == disc -> first_track) {
                    if (i == disc -> last_track)
                        disc -> first_audio_track = CDIO_CDROM_LEADOUT_TRACK;
                    else
                        ++(disc -> first_audio_track);
                }

                disc -> track[j].is_audio = FALSE;
            } else {
                disc -> track[j].is_audio = TRUE;
            }

            // Get actual track number
            disc -> track[j].num = i;

            // Get track start frame (Again, LBA)
            disc -> track[j].start_frame = cdio_get_track_lba (disc -> cdio, i);
            if (disc -> track[j].start_frame == CDIO_INVALID_LBA) {
                g_error ("Track %d has invalid start frame LBA", i);
                return FALSE;
            }

            // Convert to time
            if (!cdio_get_track_msf (disc -> cdio, i, &msf)) {
                g_error ("Track %d has invalid MSF", i);
                return FALSE;
            }
            disc -> track[j].start_pos.mins = cdio_from_bcd8 (msf.m);
            disc -> track[j].start_pos.secs = cdio_from_bcd8 (msf.s);

            // End frame
			disc -> track[j].end_frame = cdio_lsn_to_lba (cdio_get_track_last_lsn (disc -> cdio, i));
            if (disc -> track[j].end_frame == CDIO_INVALID_LBA) {
                g_error ("Track %d has invalid end frame LBA", i);
                return FALSE;
            }
            /* Compensate for the gap before a data track */
			// FIXME: Necessary?
//			if ((ginfo -> rip_track < (ginfo -> disc.num_tracks - 1) &&
//					!(ginfo -> disc).track[ginfo -> rip_track + 1].is_audio &&
//					(ginfo -> end_sector - ginfo -> start_sector) > 11399)) {
//				ginfo -> end_sector -= 11400;
//			}

            // Pre-gap, not used anywhere for the moment
			disc -> track[j].pregap_start_frame = cdio_get_track_pregap_lba (disc -> cdio, i);
            if (disc -> track[j].pregap_start_frame == CDIO_INVALID_LBA) {
                g_warning ("Track %d has invalid pre-gap frame LBA", i);
                //return FALSE;
            }

            // Get track length - What about pre-gap? Should we consider it? See cdio_get_track_pregap_lba()
            disc -> track[j].length.frames = cdio_get_track_sec_count (disc -> cdio, i);
            if (disc -> track[j].length.frames == 0) {
                g_error ("Track %d has invalid length", i);
                return FALSE;
            }
            int len = disc -> track[j].length.frames / CDIO_CD_FRAMES_PER_SEC;
            disc -> track[j].length.mins = len / 60;
            disc -> track[j].length.secs = len % 60;
            g_debug ("-- Track %d frames: %d-%d [MS(F): %02d:%02d], length: %02d:%02d, pregap: %d", i, disc -> track[j].start_frame, disc -> track[j].end_frame, disc -> track[j].start_pos.mins, disc -> track[j].start_pos.secs, disc -> track[j].length.mins, disc -> track[j].length.secs, disc -> track[j].pregap_start_frame);

            ++j;
        }

        // Read subchannel data (i.e.: status of playing)
        cdio_subchannel_t sub;
        if (cdio_audio_read_subchannel (disc -> cdio, &sub) == DRIVER_OP_SUCCESS) {
//            g_debug ("CD drive says it's playing track %d (%d)", sub.track, sub.index);
            disc -> curr_track = sub.track;
//            disc -> curr_frame = cdio_msf_to_lba (&sub.abs_addr);
            disc -> track_time.mins = cdio_from_bcd8 (sub.rel_addr.m);
            disc -> track_time.secs = cdio_from_bcd8 (sub.rel_addr.s);
            disc -> track_time.frames = cdio_msf_to_lba (&sub.rel_addr);
            disc -> disc_time.mins = cdio_from_bcd8 (sub.abs_addr.m);
            disc -> disc_time.secs = cdio_from_bcd8 (sub.abs_addr.s);
            disc -> disc_time.frames = cdio_msf_to_lba (&sub.abs_addr);

            switch (sub.audio_status) {
                case CDIO_MMC_READ_SUB_ST_PLAY:
                    disc -> disc_mode = CDAUDIO_PLAYING;
                    break;
                case CDIO_MMC_READ_SUB_ST_PAUSED:
                    disc -> disc_mode = CDAUDIO_PAUSED;
                    break;
                case CDIO_MMC_READ_SUB_ST_COMPLETED:
                    disc -> disc_mode = CDAUDIO_COMPLETED;
                    break;
                default:
                    g_warning ("Unknown subchannel status: %d", sub.audio_status);
                    // Don't break here!
                case CDIO_MMC_READ_SUB_ST_NO_STATUS:
                    disc -> disc_mode = CDAUDIO_NOSTATUS;
                    break;
            }
        } else {
            g_error ("Cannot read subchannel data");
        }
    }

	return disc -> disc_present;
}

/* Play frames from CD */
gboolean CDPlayFrames (DiscInfo *disc, int startframe, int endframe) {
	if (!disc -> cdio) {
        return FALSE;
	} else {
	    msf_t start, end;

	    cdio_lba_to_msf (startframe, &start);
	    cdio_lba_to_msf (endframe, &end);
        return cdio_audio_play_msf (disc -> cdio, &start, &end) == DRIVER_OP_SUCCESS;
	}
}

/* Play starttrack at position pos to endtrack */
gboolean CDPlayTrackPos (DiscInfo *disc, int starttrack,
                         int endtrack, int startpos) {

	return CDPlayFrames (disc, disc -> track[starttrack - 1].start_frame +
	                     startpos * 75, endtrack >= disc -> num_tracks ?
	                     (disc -> length.mins * 60 +
	                      disc -> length.secs) * 75 :
	                     disc -> track[endtrack].start_frame - 1);
}

/* Play starttrack to endtrack */
gboolean CDPlayTrack (DiscInfo *disc, int starttrack, int endtrack) {
	return CDPlayTrackPos (disc, starttrack, endtrack, 0);
}

/* Advance (fastfwd) */
gboolean CDAdvance (DiscInfo *disc, DiscTime *time) {
	if (!disc -> cdio) {
		return FALSE;
	}

	disc -> track_time.mins += time -> mins;
	disc -> track_time.secs += time -> secs;

	if (disc -> track_time.secs > 60) {
		disc -> track_time.secs -= 60;
		disc -> track_time.mins++;
	}

	if (disc -> track_time.secs < 0) {
		disc -> track_time.secs = 60 + disc -> track_time.secs;
		disc -> track_time.mins--;
	}

	/*  If we skip back past the beginning of a track, go to the end of
	    the last track - DCV */
	if (disc -> track_time.mins < 0) {
		disc -> curr_track--;

		/*  Tried to skip past first track so go to the beginning  */
		if (disc -> curr_track == 0) {
			disc -> curr_track = 1;
			return CDPlayTrack (disc, disc -> curr_track, disc -> curr_track);
		}

		/*  Go to the end of the last track  */
		disc -> track_time.mins = disc -> track[ (disc -> curr_track) - 1].
		                        length.mins;
		disc -> track_time.secs = disc -> track[ (disc -> curr_track) - 1].
		                        length.secs;

		/*  Try again  */
		return CDAdvance (disc, time);
	}

	if ((disc -> track_time.mins ==
	        disc -> track[disc -> curr_track].start_pos.mins &&
	        disc -> track_time.secs >
	        disc -> track[disc -> curr_track].start_pos.secs)
	        || disc -> track_time.mins >
	        disc -> track[disc -> curr_track].start_pos.mins) {
		disc -> curr_track++;

		if (disc -> curr_track > disc -> num_tracks) {
			disc -> curr_track = disc -> num_tracks;
		}

		return CDPlayTrack (disc, disc -> curr_track, disc -> curr_track);
	}

	return CDPlayTrackPos (disc, disc -> curr_track, disc -> curr_track,
	                       disc -> track_time.mins * 60 +
	                       disc -> track_time.secs);
}

/* Stop the CD, if it is playing */
gboolean CDStop (DiscInfo *disc) {
	if (!disc -> cdio) {
        return FALSE;
	} else {
        return cdio_audio_stop (disc -> cdio) == DRIVER_OP_SUCCESS;
	}
}

/* Pause the CD */
gboolean CDPause (DiscInfo *disc) {
	if (!disc -> cdio) {
        return FALSE;
	} else {
        return cdio_audio_pause (disc -> cdio) == DRIVER_OP_SUCCESS;
	}
}

/* Resume playing */
gboolean CDResume (DiscInfo *disc) {
	if (!disc -> cdio) {
        return FALSE;
	} else {
        return cdio_audio_resume (disc -> cdio) == DRIVER_OP_SUCCESS;
	}
}

/* Check the tray status */
gboolean IsTrayOpen (DiscInfo *disc) {
	if (!disc -> cdio) {
        return FALSE;
	} else {
        return mmc_get_tray_status (disc -> cdio);
	}
}

/* Eject the tray */
gboolean CDEject (DiscInfo *disc) {
	if (!disc -> devname || strlen (disc -> devname) == 0) {
        return FALSE;
	} else {
        return cdio_eject_media_drive (disc -> devname) == DRIVER_OP_SUCCESS;
	}
}

/* Close the tray */
gboolean CDClose (DiscInfo *disc) {
	if (!disc -> devname || strlen (disc -> devname) == 0) {
        return FALSE;
	} else {
        return cdio_close_tray (disc -> devname, NULL) == DRIVER_OP_SUCCESS;
	}
}

gboolean CDGetVolume (DiscInfo *disc, DiscVolume *vol) {
    gboolean ret;

	if (!disc -> cdio) {
        ret = FALSE;
	} else {
        cdio_audio_volume_t cdio_vol;

        if ((ret = cdio_audio_get_volume (disc -> cdio, &cdio_vol)) == DRIVER_OP_SUCCESS) {
            // Not sure the level mapping is correct here
            vol -> vol_front.left = cdio_vol.level[0];
            vol -> vol_front.right = cdio_vol.level[1];
            vol -> vol_back.left = cdio_vol.level[2];
            vol -> vol_back.right = cdio_vol.level[3];
        }
	}

	return ret;
}

gboolean CDSetVolume (DiscInfo *disc, DiscVolume *vol) {
	if (!disc -> cdio) {
        return FALSE;
	} else if (vol -> vol_front.left > 255 || vol -> vol_front.left < 0 ||
               vol -> vol_front.right > 255 || vol -> vol_front.right < 0 ||
               vol -> vol_back.left > 255 || vol -> vol_back.left < 0 ||
               vol -> vol_back.right > 255 || vol -> vol_back.right < 0) {
        return FALSE;
	} else {
	    cdio_audio_volume_t cdio_vol;

	    // Not sure the level mapping is correct here
        cdio_vol.level[0] = vol -> vol_front.left;
        cdio_vol.level[1] = vol -> vol_front.right;
        cdio_vol.level[2] = vol -> vol_back.left;
        cdio_vol.level[3] = vol -> vol_back.right;

        return cdio_audio_set_volume (disc -> cdio, &cdio_vol) == DRIVER_OP_SUCCESS;
	}
}
