
#include <glib.h>
#include <glib/gi18n.h>
#include <sndfile.h>
#include "gain_analysis.h"
#include "encoder.h"

typedef struct {
    gboolean do_gain_calc;
    SNDFILE *outfile;
} encoder_callback_data;


// GError stuff

#define GRIP_ENCODER_ERROR grip_encoder_error_quark ()

GQuark grip_encoder_error_quark (void) {
    return g_quark_from_static_string ("grip-encoder-error-quark");
}

enum GripEncoderError {
    GRIP_ENCODER_ERROR_INVALIDFORMAT,
    GRIP_ENCODER_ERROR_UNSUPPORTEDFORMAT,
    GRIP_ENCODER_ERROR_OUTFILE
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
gboolean encoder_callback (gint16 *buffer, gsize bufsize, gpointer user_data) {
//    encoder_callback_data *data = (encoder_callback_data *) user_data;

    g_debug ("encoder_callback()");

//    if (data -> do_gain_calc)
//        GainCalc ((char *) buffer);

    if (sf_write_short ((SNDFILE *) user_data, buffer, bufsize / 2) != bufsize / 2) {
//        fprintf (stderr, "Error writing output: %s", sf_strerror (data -> outfile));

//        sf_close (file);
//        cdda_close (d);
//        paranoia_free (p);

        g_debug ("encoder_callback() failed");
        return FALSE;
    }

    g_debug ("encoder_callback() returning");
    return TRUE;
}

gpointer encoder_init (gchar *filename, encoder_options *opts, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    gpointer ret;

    // We only support CD quality output
    SF_INFO sfinfo = {0};
	sfinfo.samplerate = 44100;
	sfinfo.channels = 2;

	switch (opts -> format) {
        case FILEFMT_WAV:
            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            break;
        case FILEFMT_VORBIS:
            sfinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            //sf_command (sndfile, SFC_SET_COMPRESSION_LEVEL, &quality, sizeof (quality));
            break;
        case FILEFMT_FLAC:
            sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
            break;
        default:
            *error = g_error_new_literal (GRIP_ENCODER_ERROR, GRIP_ENCODER_ERROR_UNSUPPORTEDFORMAT, _("The requested encoder format is not (yet) supported"));
            return NULL;
            break;
	}

    ret = NULL;
    if (!sf_format_check (&sfinfo)) {
        *error = g_error_new_literal (GRIP_ENCODER_ERROR, GRIP_ENCODER_ERROR_INVALIDFORMAT, _("Invalid encoder parameters"));
    } else {
        // OK, open output file
        SNDFILE *file;
        if (!(file = sf_open (filename, SFM_WRITE, &sfinfo))) {
            *error = g_error_new_literal (GRIP_ENCODER_ERROR, GRIP_ENCODER_ERROR_OUTFILE, _("Unable to open output file"));
        } else {
            // libsndfile is ready!
            ret = file;
            g_debug ("libsndfile ready!");
        }
    }

    return ret;
}

gboolean encoder_close (gpointer encoder_data, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    SNDFILE *file = (SNDFILE *) encoder_data;

    return sf_close (file) == 0;
}
