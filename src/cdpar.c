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
#include <sndfile.h>
#include "gain_analysis.h"
#include "grip.h"
#include "cdpar.h"

//static void CDPCallback (long inpos, int function);
//static void GainCalc (char *buffer);

typedef gboolean (*cdrip_callback) (gint16 *buffer, gsize bufsize, void *user_data);

typedef struct {
    cdrom_drive *d;
    cdrom_paranoia *p;
    gboolean do_gain_calc;
    SNDFILE *outfile;
    long first_sector;
    long last_sector;
    cdrip_callback callback;
} cdrip_callback_data;

/* Ugly hack because we can't pass user data to the callback */
int *global_rip_smile_level;
FILE *global_output_fp;


// GError stuff

#define GRIP_RIP_ERROR grip_rip_error_quark ()

GQuark grip_rip_error_quark (void) {
    return g_quark_from_static_string ("grip-rip-error-quark");
}

enum GripRipError {
    GRIP_RIP_ERROR_INVALIDENCPARAMS,
    GRIP_RIP_ERROR_CANTOPENDRIVE,
    GRIP_RIP_ERROR_NOAUDIOTRACK,
    GRIP_RIP_ERROR_OUTFILE
};


/* Do the replay gain calculation on a sector */
static void GainCalc (char *buffer) {
	static Float_t l_samples[588];
	static Float_t r_samples[588];
	long count;
	short *data;

	data = (short *) buffer;

	for (count = 0; count < 588; count++) {
		l_samples[count] = (Float_t) data[count * 2];
		r_samples[count] = (Float_t) data[ (count * 2) + 1];
	}

	AnalyzeSamples (l_samples, r_samples, 588, 2);
}


/** \brief Function that gets called whenever a block of audio data has been read from the CD and is read for processing.
 *
 * \param Block of audio data (this should contain pairs of L/R samples)
 * \param Size of block
 * \return FALSE if rip must be aborted, TRUE otherwise.
 *
 */
static gboolean cdrip_callback_func (gint16 *buffer, gsize bufsize, void *user_data) {
    cdrip_callback_data *data = (cdrip_callback_data *) user_data;

    g_debug ("cdrip_callback_func()");

    if (data -> do_gain_calc)
        GainCalc ((char *) buffer);

    if (sf_write_short (data -> outfile, buffer, bufsize / 2) != bufsize / 2) {
        fprintf (stderr, "Error writing output: %s", sf_strerror (data -> outfile));

//        sf_close (file);
//        cdda_close (d);
//        paranoia_free (p);

        return FALSE;
    }

    return TRUE;
}

static void CDPCallback (long inpos, int function) {
	static long c_sector = 0/*,v_sector=0*/;
	static int last = 0;
	static long lasttime = 0;
	long sector, osector = 0;
	struct timeval thistime;
	//  static char heartbeat=' ';
	static int overlap = 0;
	static int slevel = 0;
	static int slast = 0;
	static int stimeout = 0;
	long test;

	osector = inpos;
	sector = inpos / CD_FRAMEWORDS;

	if (function == -1) {
		last = 8;
		//    heartbeat='*';
		slevel = 0;
		//    v_sector=sector;
	} else
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

			case PARANOIA_CB_READ:
				if (sector > c_sector) {
					c_sector = sector;
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
				g_assert_not_reached();
				break;
		}


	gettimeofday (&thistime, NULL);
	test = thistime.tv_sec * 10 + thistime.tv_usec / 100000;

	if (lasttime != test || function == -1 || slast != slevel) {
		if (lasttime != test || function == -1) {
			last++;
			lasttime = test;

			if (last > 7) {
				last = 0;
			}

			stimeout++;

			switch (last) {
				case 0:
					//  heartbeat=' ';
					break;

				case 1:
				case 7:
					//  heartbeat='.';
					break;

				case 2:
				case 6:
					//  heartbeat='o';
					break;

				case 3:
				case 5:
					//  heartbeat='0';
					break;

				case 4:
					//  heartbeat='O';
					break;
			}

			//      if(function==-1)
			//  heartbeat='*';

		}

		if (slast != slevel) {
			stimeout = 0;
		}

		slast = slevel;
	}

	if (slevel < 8 && slevel > 0) {
		*global_rip_smile_level = slevel - 1;
	} else {
		*global_rip_smile_level = 0;
	}
}

/** \brief Rip thread function
 *
 * \param
 * \param
 * \return TRUE if ripping succeeded, FALSE otherwise (As this is actually a gpointer, use GPOINTER_TO_INT() to convert)
 *
 */
static gpointer rip_thread_func (gpointer user_data) {
    cdrip_callback_data *data = (cdrip_callback_data *) user_data;
    gfloat rip_percent_done;

    paranoia_seek (data -> p, data -> first_sector, SEEK_SET);

    long cursor;
    for (cursor = data -> first_sector; cursor <= data -> last_sector; ++cursor) {
		/* read a sector */
		gint16 *readbuf = paranoia_read (data -> p, CDPCallback);
		char *err = cdda_errors (data -> d);
		char *mes = cdda_messages (data -> d);

		rip_percent_done = (gfloat) cursor / (gfloat) data -> last_sector;
		g_debug ("Rip progress: %.0f", rip_percent_done);

		if (mes || err)
			fprintf (stderr, "\r                               "
			         "                                           \r%s%s\n",
			         mes ? mes : "", err ? err : "");

		if (err)
			free (err);

		if (mes)
			free (mes);

//		if (*stop_thread_rip_now) {
//			*stop_thread_rip_now = FALSE;
//
//			cdda_close (d);
//			paranoia_free (p);
//
//			return FALSE;
//		}

		if (readbuf == NULL) {
			fprintf (stderr, "\nparanoia_read: Unrecoverable error, bailing.\n");
			paranoia_seek (data -> p, data -> last_sector + 1, SEEK_SET);           // Why?
			break;
		} else {
            // OK, data is ready, send to callback
            data -> callback (readbuf, CD_FRAMESIZE_RAW, data);
		}
	}

	return GUINT_TO_POINTER (cursor > data -> last_sector);
}


/** \brief Starts the CD-rip thread
 *
 * \param
 * \param
 * \return TRUE if the thread was started successfully, FALSE otherwise with error set
 *
 */
gboolean rip_start (GripInfo *ginfo, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_debug (_("Starting Rip"));

	// Init encoder
    SF_INFO sfinfo = {0};
	sfinfo.samplerate = 44100;
	sfinfo.channels = 2;
	sfinfo.format = (SF_FORMAT_OGG | SF_FORMAT_VORBIS);

    if (!sf_format_check (&sfinfo)) {
        *error = g_error_new_literal (GRIP_RIP_ERROR, GRIP_RIP_ERROR_CANTOPENDRIVE, _("Invalid encoder parameters"));
        return FALSE;
    } else {
        int paranoia_mode;
        int dup_output_fd;
        FILE *output_fp;

        paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

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

        ginfo -> rip_smile_level = 0;

        dup_output_fd = dup (GetStatusWindowPipe (ginfo -> gui_info.rip_status_window));
        output_fp = fdopen (dup_output_fd, "w");
        setlinebuf (output_fp);

        //////////////////////

    //	CDPRip (ginfo -> cd_device, ginfo -> force_scsi, ginfo -> rip_track + 1,
    //	        ginfo -> start_sector,
    //	        ginfo -> end_sector, ginfo -> ripfile, paranoia_mode,
    //	        & (ginfo -> rip_smile_level), & (ginfo -> rip_percent_done),
    //	        & (ginfo -> stop_thread_rip_now), ginfo -> calc_gain,
    //	        output_fp);

    //	int force_cdrom_endian = -1;
    //	int force_cdrom_sectors = -1;
    //	int force_cdrom_overlap = -1;

        /* full paranoia, but allow skipping */
        int verbose = CDDA_MESSAGE_FORGETIT;
        int track = ginfo -> rip_track + 1;

        global_rip_smile_level = &(ginfo -> rip_smile_level);
        global_output_fp = output_fp;

        /* Query the cdrom/disc; */
        cdrom_drive *d;
        if (ginfo -> force_scsi && *ginfo -> force_scsi) {
            d = cdda_identify_scsi (ginfo -> force_scsi, ginfo -> cd_device, verbose, NULL);
        } else {
            d = cdda_identify (ginfo -> cd_device, verbose, NULL);
        }

        if (!d) {
            *error = g_error_new_literal (GRIP_RIP_ERROR, GRIP_RIP_ERROR_CANTOPENDRIVE, _("Unable to open cdrom drive"));
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
                fprintf (output_fp,
                         "\nUnable to open disc.  Is there an audio CD in the drive?");
                cdda_close (d);
                return FALSE;

            case -6:
                fprintf (output_fp,
                         "\nCdparanoia could not find a way to read audio from this drive.");
                cdda_close (d);
                return FALSE;

            case 0:
                break;

            default:
                fprintf (output_fp, "\nUnable to open disc.");
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
            *error = g_error_new_literal (GRIP_RIP_ERROR, GRIP_RIP_ERROR_NOAUDIOTRACK, _("Selected track is not an audio track"));
            cdda_close (d);
            return FALSE;
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

        // OK, paranoia is ready, open output file
        SNDFILE *file;
        if (!(file = sf_open (ginfo -> ripfile, SFM_WRITE, &sfinfo))) {
            *error = g_error_new_literal (GRIP_RIP_ERROR, GRIP_RIP_ERROR_OUTFILE, _("Unable to open output file"));
            cdda_close (d);
            paranoia_free (p);

            return FALSE;
        }

        /* Off we go! */
        cdrip_callback_data cbdata;
        cbdata.d = d;
        cbdata.p = p;
        cbdata.do_gain_calc = ginfo -> calc_gain;
        cbdata.outfile = file;
        cbdata.first_sector = ginfo -> start_sector + cdda_track_firstsector (d, track);
        cbdata.last_sector = ginfo -> end_sector + cdda_track_firstsector (d, track);
        cbdata.callback = cdrip_callback_func;

        g_debug ("Starting Rip Thread");
        ginfo -> in_rip_thread = TRUE;
        GThread *rip_thread = g_thread_new ("Grip Rip Thread", rip_thread_func, &cbdata);

        gboolean rip_ok = GPOINTER_TO_INT (g_thread_join (rip_thread));
        ginfo -> in_rip_thread = FALSE;
        g_debug ("Rip Thread Finished with %d", rip_ok);

        // Finished
//        CDPCallback (cursor * (CD_FRAMESIZE_RAW / 2) - 1, -1);
        sf_close (file);

        paranoia_free (p);

        cdda_close (d);

        fclose (output_fp);
    }

	return TRUE;
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

