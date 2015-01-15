#include <glib.h>
#include <glib/gi18n.h>
#include <sndfile.h>
#include <encoder.h>

typedef struct {
    int sf_format;
} fmtdata;

static fmtdata format_data[] = {
    {SF_FORMAT_WAV | SF_FORMAT_PCM_16},
    {SF_FORMAT_OGG | SF_FORMAT_VORBIS},
    {SF_FORMAT_FLAC | SF_FORMAT_PCM_16}
};

static supported_format sndfile_formats[] = {
    {"WAV", "wav", &(format_data[0])},
    {"OGG Vorbis", "ogg", &(format_data[1])},
    {"FLAC", "flac", &(format_data[2])},
    {NULL}
};



// GError stuff
#define GRIP_SFENC_ERROR grip_sfenc_error_quark ()

GQuark grip_sfenc_error_quark (void) {
    return g_quark_from_static_string ("grip-sfenc-error-quark");
}

enum GripSFEncError {
    GRIP_SFENC_ERROR_INVALIDFORMAT,
    GRIP_SFENC_ERROR_OUTFILE
};


gpointer encoder_sndfile_init (supported_format *fmt, FILE *fp, gpointer user_data, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    gpointer ret;

//    g_debug ("encoder_sndfile_init()");

    fmtdata *fmtd = (fmtdata *) fmt -> data;
    g_assert (fmtd);

    // We only support CD quality output
    SF_INFO sfinfo = {0};
	sfinfo.samplerate = 44100;
	sfinfo.channels = 2;
	sfinfo.format = fmtd -> sf_format;

    ret = NULL;
    if (!sf_format_check (&sfinfo)) {
        g_set_error_literal (error, GRIP_SFENC_ERROR, GRIP_SFENC_ERROR_INVALIDFORMAT, _("Invalid encoder parameters"));
    } else {
        // OK, open output file
        SNDFILE *file;
        int fd = fileno (fp);
        if (fd < 0 || !(file = sf_open_fd (fd, SFM_WRITE, &sfinfo, FALSE))) {
            g_set_error (error, GRIP_SFENC_ERROR, GRIP_SFENC_ERROR_OUTFILE, _("Unable to open output file: %s"), sf_strerror (file));
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
supported_encoder encoder = {
    "libsndfile Encoder",
    sndfile_formats,
    encoder_sndfile_init,
    encoder_sndfile_close,
    encoder_sndfile_callback
};
