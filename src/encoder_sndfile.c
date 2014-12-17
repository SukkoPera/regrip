#include <glib.h>
#include <glib/gi18n.h>
#include <sndfile.h>
#include "encoder_sndfile.h"


//typedef enum {
//    FILEFMT_WAV,
//    FILEFMT_VORBIS,
//    FILEFMT_FLAC,
////    FILEFMT_MP3
//} file_format;

//typedef struct {
//    gchar *extension;
//    int format;
//} sndfile_encoder;

static supported_format sndfile_formats[] = {
    {"WAV", GINT_TO_POINTER (SF_FORMAT_WAV | SF_FORMAT_PCM_16)},
    {"OGG Vorbis", GINT_TO_POINTER (SF_FORMAT_OGG | SF_FORMAT_VORBIS)},
    {"FLAC", GINT_TO_POINTER (SF_FORMAT_FLAC | SF_FORMAT_PCM_16)},
    {NULL}
};


//typedef struct {
//    gboolean do_gain_calc;
//    SNDFILE *outfile;
//} encoder_callback_data;


// GError stuff
#define GRIP_SFENC_ERROR grip_sfenc_error_quark ()

GQuark grip_sfenc_error_quark (void) {
    return g_quark_from_static_string ("grip-sfenc-error-quark");
}

enum GripSFEncError {
    GRIP_SFENC_ERROR_INVALIDFORMAT,
    GRIP_SFENC_ERROR_UNSUPPORTEDFORMAT,
    GRIP_SFENC_ERROR_OUTFILE
};


gpointer encoder_sndfile_init (gpointer *fmt, gchar *filename, gpointer opts, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    gpointer ret;

//    g_debug ("encoder_sndfile_init()");

    // We only support CD quality output
    SF_INFO sfinfo = {0};
	sfinfo.samplerate = 44100;
	sfinfo.channels = 2;
	sfinfo.format = GPOINTER_TO_INT (fmt);

//	switch (opts -> format) {
//        case FILEFMT_WAV:
//            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
//            break;
//        case FILEFMT_VORBIS:
//            sfinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
//            //sf_command (sndfile, SFC_SET_COMPRESSION_LEVEL, &quality, sizeof (quality));
//            break;
//        case FILEFMT_FLAC:
//            sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
//            break;
//        default:
//            *error = g_error_new_literal (GRIP_SFENC_ERROR, GRIP_SFENC_ERROR_UNSUPPORTEDFORMAT, _("The requested encoder format is not (yet) supported"));
//            return NULL;
//            break;
//	}

    ret = NULL;
    if (!sf_format_check (&sfinfo)) {
        *error = g_error_new_literal (GRIP_SFENC_ERROR, GRIP_SFENC_ERROR_INVALIDFORMAT, _("Invalid encoder parameters"));
    } else {
        // OK, open output file
        SNDFILE *file;
        if (!(file = sf_open (filename, SFM_WRITE, &sfinfo))) {
            *error = g_error_new_literal (GRIP_SFENC_ERROR, GRIP_SFENC_ERROR_OUTFILE, _("Unable to open output file"));
        } else {
            // libsndfile is ready!
            ret = file;
//            g_debug ("libsndfile ready!");
        }
    }

    return ret;
}

gboolean encoder_sndfile_close (gpointer user_data, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

//    g_debug ("encoder_sndfile_close()");

    SNDFILE *file = (SNDFILE *) user_data;

    return sf_close (file) == 0;
}

/** \brief Function that gets called whenever a block of audio data has been read from the CD and is ready for processing.
 *
 * \param Block of audio data (this should contain pairs of L/R samples)
 * \param Size of block in bytes
 * \return FALSE if rip must be aborted, TRUE otherwise.
 *
 */
gboolean encoder_sndfile_callback (gint16 *buffer, gsize bufsize, gpointer user_data) {
//    encoder_callback_data *data = (encoder_callback_data *) user_data;

//    g_debug ("encoder_callback()");

    if (sf_write_short ((SNDFILE *) user_data, buffer, bufsize / 2) != bufsize / 2) {
//        fprintf (stderr, "Error writing output: %s", sf_strerror (data -> outfile));

        g_warning ("sf_write_short() failed: %s", sf_strerror ((SNDFILE *) user_data));
        return FALSE;
    }

    return TRUE;
}

// Encoder registration
supported_encoder sndfile_encoder = {
    "libsndfile Encoder",
    sndfile_formats,
    encoder_sndfile_init,
    encoder_sndfile_close,
    encoder_sndfile_callback
};
