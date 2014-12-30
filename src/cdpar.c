/* cdpar.c -- routines for interacting with the Paranoia library
 *
 * Based on main.c from the cdparanoia distribution
 *  (C) 1998 Monty <xiphmont@mit.edu>
 *
 * All changes Copyright 1999-2004 by Mike Oliphant (grip@nostatic.org)
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
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <cdda_interface.h>
#include <cdda_paranoia.h>
#include "grip.h"
#include "cdpar.h"


typedef struct {
    GripInfo *ginfo;
    cdrom_drive *d;
    cdrom_paranoia *p;
    long first_sector;
    long last_sector;
    cdrip_callback callback;
    gpointer callback_data;
} rip_thread_data;

/* Ugly hack because we can't pass user data to the callback */
static int *global_rip_smile_level;
static FILE *global_output_fp;
static int skipped_flag = 0;

// GError stuff

#define GRIP_RIP_ERROR grip_rip_error_quark ()

GQuark grip_rip_error_quark (void) {
    return g_quark_from_static_string ("grip-rip-error-quark");
}

enum GripRipError {
    GRIP_RIP_ERROR_CANTOPENDRIVE,
    GRIP_RIP_ERROR_NOAUDIOTRACK,
    GRIP_RIP_ERROR_NOAUDIOCD,
    GRIP_RIP_ERROR_CANTREADAUDIO,
    GRIP_RIP_ERROR_CANTOPENDISC
};


const char * const get_smilie (int slevel) {
    char *smilie = NULL;

	switch (slevel) {
        case 0:  /* finished, or no jitter */
            if (skipped_flag)
                smilie = "8-X";
            else
                smilie = ":^D";
            break;
        case 1:  /* normal.  no atom, low jitter */
            smilie = ":-)";
            break;
        case 2:  /* normal, overlap > 1 */
            smilie = ":-|";
            break;
        case 4:  /* drift */
            smilie = ":-/";
            break;
        case 3:  /* unreported loss of streaming */
            smilie = ":-P";
            break;
        case 5:  /* dropped/duped bytes */
            smilie = "8-|";
            break;
        case 6:  /* scsi error */
            smilie = ":-0";
            break;
        case 7:  /* scratch */
            smilie = ":-(";
            break;
        case 8:  /* skip */
            smilie = ";-(";
            skipped_flag = 1;
            break;
        default:
            g_assert_not_reached ();
            break;
	}

	return smilie;
}

static void cdparanoia_callback (long inpos, int function) {
	static long lasttime = 0;
	long osector = 0;
	struct timeval thistime;
	static int overlap = 0;
	static int slevel = 0;
	static int slast = 0;
	static int stimeout = 0;
	long test;

	osector = inpos;

	if (function == -1) {
		slevel = 0;
	} else {
		switch (function) {
			case PARANOIA_CB_VERIFY:
				if (stimeout >= 30) {
					if (overlap > CD_FRAMEWORDS) {
						slevel = 2;
					} else {
						slevel = 1;
					}
				}
				break;

			case PARANOIA_CB_FIXUP_EDGE:
				if (stimeout >= 5) {
					if (overlap > CD_FRAMEWORDS) {
						slevel = 2;
					} else {
						slevel = 1;
					}
				}
				break;

			case PARANOIA_CB_FIXUP_ATOM:
				if (slevel < 3 || stimeout > 5) {
					slevel = 3;
				}
				break;

			case PARANOIA_CB_READERR:
				slevel = 6;
				break;

			case PARANOIA_CB_SKIP:
				slevel = 8;
				break;

			case PARANOIA_CB_OVERLAP:
				overlap = osector;
				break;

			case PARANOIA_CB_SCRATCH:
				slevel = 7;
				break;

			case PARANOIA_CB_DRIFT:
				if (slevel < 4 || stimeout > 5) {
					slevel = 4;
				}
				break;

			case PARANOIA_CB_FIXUP_DROPPED:
			case PARANOIA_CB_FIXUP_DUPED:
				slevel = 5;
				break;

			default:
				// Never mind
				break;
		}
	}

	gettimeofday (&thistime, NULL);
	test = thistime.tv_sec * 10 + thistime.tv_usec / 100000;

	if (lasttime != test || function == -1 || slast != slevel) {
		if (lasttime != test || function == -1) {
			lasttime = test;
			stimeout++;
		}

		if (slast != slevel) {
			stimeout = 0;
		}

		slast = slevel;
	}

    *global_rip_smile_level = slevel;
}

/** \brief Rip thread function
 *
 * \param
 * \param
 * \return TRUE if ripping succeeded, FALSE otherwise (As this is actually a gpointer, use GPOINTER_TO_INT() to convert)
 *
 */
static gpointer rip_thread_func (gpointer user_data) {
    rip_thread_data *data = (rip_thread_data *) user_data;
    long done, todo;
    gfloat percent;

    paranoia_seek (data -> p, data -> first_sector, SEEK_SET);

    todo = data -> last_sector - data -> first_sector + 1;

    data -> ginfo -> in_rip_thread = TRUE;

    /* Workaround for drives that spin up slowly */
    if (data -> ginfo -> delay_before_rip) {
        sleep (5);          // FIXME: Make 5 a constant or even an option
    }

    skipped_flag = 0;

    long cursor;
    for (cursor = data -> first_sector; cursor <= data -> last_sector; ++cursor) {
		/* Read a sector */
		gint16 *readbuf = paranoia_read (data -> p, cdparanoia_callback);

		done = cursor - data -> first_sector;
		percent = (gfloat) done / (gfloat) todo;
//		g_debug ("Rip progress: %.02f", percent);

		char *mes = cdda_messages (data -> d);
		if (mes) {
            g_warning ("CDParanoia message: %s", mes);
            free (mes);
		}

		char *err = cdda_errors (data -> d);
		if (err) {
            g_warning ("CDParanoia error: %s", err);
            free (err);
		}

		if (readbuf == NULL) {
			fprintf (stderr, "\nparanoia_read: Unrecoverable error, bailing.\n");
			paranoia_seek (data -> p, data -> last_sector + 1, SEEK_SET);           // Why?
			break;
		} else {
            // OK, data is ready, send to callback
            if (!data -> callback (readbuf, CD_FRAMESIZE_RAW, percent, *global_rip_smile_level, data -> callback_data)) {
                g_debug ("Callback requested to abort rip");
                break;
            }
		}

		skipped_flag = 0;
	}

    // Finished
    cdparanoia_callback (cursor * (CD_FRAMESIZE_RAW / 2) - 1, -1);

    // Clean up
    paranoia_free (data -> p);
    cdda_close (data -> d);
//    fclose (output_fp);           // FIXME

    data -> ginfo -> in_rip_thread = FALSE;

    gboolean ret = cursor > data -> last_sector;

    g_free (data);

	return GUINT_TO_POINTER (ret);
}


/** \brief Starts the CD-rip thread
 *
 * \param
 * \param
 * \return TRUE if the thread was started successfully, FALSE otherwise with error set
 *
 */
gboolean rip_start (GripInfo *ginfo, cdrip_callback callback, gpointer callback_data, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_debug (_("Starting rip"));
    g_assert (ginfo -> rip_thread == NULL);

    // Prepare log windows fd
    int dup_output_fd = dup (GetStatusWindowPipe (ginfo -> gui_info.rip_status_window));
    FILE *output_fp = fdopen (dup_output_fd, "w");
    setlinebuf (output_fp);
    global_output_fp = output_fp;

//	int force_cdrom_endian = -1;
//	int force_cdrom_sectors = -1;
//	int force_cdrom_overlap = -1;

    int verbose = CDDA_MESSAGE_FORGETIT;
    int track = ginfo -> rip_track + 1;

    ginfo -> rip_smile_level = 0;
    global_rip_smile_level = &(ginfo -> rip_smile_level);

    /* Query the cdrom/disc; */
    cdrom_drive *d;
    if (ginfo -> force_scsi && *ginfo -> force_scsi) {
        d = cdda_identify_scsi (ginfo -> force_scsi, ginfo -> cd_device, verbose, NULL);
    } else {
        d = cdda_identify (ginfo -> cd_device, verbose, NULL);
    }

    if (!d) {
        g_set_error_literal (error, GRIP_RIP_ERROR, GRIP_RIP_ERROR_CANTOPENDRIVE, _("Unable to open cdrom drive"));
        return FALSE;
    }

    if (verbose) {
        cdda_verbose_set (d, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);
    } else {
        cdda_verbose_set (d, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_FORGETIT);
    }

#if 0
    /* possibly force hand on endianness of drive, sector request size */
    if (force_cdrom_endian != -1) {
        d -> bigendianp = force_cdrom_endian;

        switch (force_cdrom_endian) {
            case 0:
                fprintf (output_fp,
                         "Forcing CDROM sense to little-endian; ignoring preset and autosense");
                break;

            case 1:
                fprintf (output_fp,
                         "Forcing CDROM sense to big-endian; ignoring preset and autosense");
                break;
        }
    }

    if (force_cdrom_sectors != -1) {
        if (force_cdrom_sectors < 0 || force_cdrom_sectors > 100) {
            fprintf (output_fp, "Default sector read size must be 1<= n <= 100\n");
            cdda_close (d);

            return FALSE;
        }

        fprintf (output_fp, "Forcing default to read %d sectors; "
                 "ignoring preset and autosense", force_cdrom_sectors);

        d -> nsectors = force_cdrom_sectors;
        d -> bigbuff = force_cdrom_sectors * CD_FRAMESIZE_RAW;
    }

    if (force_cdrom_overlap != -1) {
        if (force_cdrom_overlap < 0 || force_cdrom_overlap > 75) {
            fprintf (output_fp, "Search overlap sectors must be 0<= n <=75\n");
            cdda_close (d);

            return FALSE;
        }

        fprintf (output_fp, "Forcing search overlap to %d sectors; "
                 "ignoring autosense", force_cdrom_overlap);
    }
#endif

    switch (cdda_open (d)) {
        case -2:
        case -3:
        case -4:
        case -5:
            g_set_error_literal (error, GRIP_RIP_ERROR, GRIP_RIP_ERROR_NOAUDIOCD, _("Unable to open disc, is there an audio CD in the drive?"));
            cdda_close (d);
            return FALSE;

        case -6:
            g_set_error_literal (error, GRIP_RIP_ERROR, GRIP_RIP_ERROR_CANTREADAUDIO, _("CDParanoia could not find a way to read audio from this drive."));
            cdda_close (d);
            return FALSE;

        case 0:
            // Allright!
            break;

        default:
            g_set_error_literal (error, GRIP_RIP_ERROR, GRIP_RIP_ERROR_CANTOPENDISC, _("Unable to open disc."));
            cdda_close (d);
            return FALSE;
    }

    if (d -> interface == GENERIC_SCSI && d -> bigbuff <= CD_FRAMESIZE_RAW) {
        fprintf (output_fp,
                 "WARNING: You kernel does not have generic SCSI 'SG_BIG_BUFF'\n"
                 "         set, or it is set to a very small value.  Paranoia\n"
                 "         will only be able to perform single sector reads\n"
                 "         making it very unlikely Paranoia can work.\n\n"
                 "         To correct this problem, the SG_BIG_BUFF define\n"
                 "         must be set in /usr/src/linux/include/scsi/sg.h\n"
                 "         by placing, for example, the following line just\n"
                 "         before the last #endif:\n\n"
                 "         #define SG_BIG_BUFF 65536\n\n"
                 "         and then recompiling the kernel.\n\n"
                 "         Attempting to continue...\n\n");
    }

    if (d -> nsectors == 1) {
        fprintf (output_fp,
                 "WARNING: The autosensed/selected sectors per read value is\n"
                 "         one sector, making it very unlikely Paranoia can \n"
                 "         work.\n\n"
                 "         Attempting to continue...\n\n");
    }

    if (!cdda_track_audiop (d, track)) {
        g_set_error_literal (error, GRIP_RIP_ERROR, GRIP_RIP_ERROR_NOAUDIOTRACK, _("Selected track is not an audio track"));
        cdda_close (d);
        return FALSE;
    }

    // Init CDParanoia - full paranoia, but allow skipping
    int paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

    if (ginfo -> disable_paranoia) {
        paranoia_mode = PARANOIA_MODE_DISABLE;
    } else if (ginfo -> disable_extra_paranoia) {
        paranoia_mode |= PARANOIA_MODE_OVERLAP;
        paranoia_mode &= ~PARANOIA_MODE_VERIFY;
    }

    if (ginfo -> disable_scratch_detect)
        paranoia_mode &=
            ~(PARANOIA_MODE_SCRATCH | PARANOIA_MODE_REPAIR);

    if (ginfo -> disable_scratch_repair) {
        paranoia_mode &= ~PARANOIA_MODE_REPAIR;
    }

    cdrom_paranoia *p = paranoia_init (d);
    paranoia_modeset (p, paranoia_mode);

//	if (force_cdrom_overlap != -1) {
//		paranoia_overlapset (p, force_cdrom_overlap);
//	}

    if (verbose) {
        cdda_verbose_set (d, CDDA_MESSAGE_LOGIT, CDDA_MESSAGE_LOGIT);
    } else {
        cdda_verbose_set (d, CDDA_MESSAGE_FORGETIT, CDDA_MESSAGE_FORGETIT);
    }

    /* this is probably a good idea in general */
    /*  seteuid(getuid());
        setegid(getgid());*/

    /* Off we go! */
    rip_thread_data *ripdata = g_new (rip_thread_data, 1);
    g_assert (ripdata);
    ripdata -> ginfo = ginfo;
    ripdata -> d = d;
    ripdata -> p = p;
    ripdata -> first_sector = ginfo -> start_sector + cdda_track_firstsector (d, track);
    ripdata -> last_sector = ginfo -> end_sector + cdda_track_firstsector (d, track);
    ripdata -> callback = callback;
    ripdata -> callback_data = callback_data;

    g_debug ("Starting rip thread");
    ginfo -> rip_thread = g_thread_try_new ("Grip Rip Thread", rip_thread_func, ripdata, error);
    if (ginfo -> rip_thread) {
        g_debug ("Rip thread started");
    } else {
        g_warning ("Cannot start rip thread: %s", (*error) -> message);
        g_prefix_error (error, "Cannot start rip thread");
    }

	return ginfo -> rip_thread != NULL;
}


//gboolean rip_start (char *device, char *generic_scsi_device, int track,
//                 long first_sector, long last_sector,
//                 char *outfile, int paranoia_mode, int *rip_smile_level,
//                 gfloat *rip_percent_done, gboolean *stop_thread_rip_now,
//                 gboolean do_gain_calc, FILE *output_fp) {
//
//
//
//	return TRUE;
//}

