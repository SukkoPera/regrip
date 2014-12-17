#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <faac.h>
#include "encoder_faac.h"

static supported_format faac_formats[] = {
    {"AAC", NULL},
    {NULL}
};


// GError stuff
#define GRIP_FAACENC_ERROR faac_encoder_error_quark ()

GQuark faac_encoder_error_quark (void) {
    return g_quark_from_static_string ("grip-encoder-error-quark");
}

enum GripFAACEncError {
    GRIP_FAACENC_ERROR_NOMEM,
    GRIP_FAACENC_ERROR_INIT,
    GRIP_FAACENC_ERROR_INVALIDFORMAT,
//    GRIP_FAACENC_ERROR_UNSUPPORTEDFORMAT,
    GRIP_FAACENC_ERROR_OUTFILE
};


typedef struct {
	FILE *fout;
	faacEncHandle codec;
	gulong input_samples;       // Number of 16-bit samples that are consumed by each call to faacEncEncode()
	gulong outbufsize;
	guint8 *outbuf;
	gsize buffer_avail;
} encoder_faac_data;


gpointer encoder_faac_init (gpointer *fmt, gchar *filename, gpointer opts, GError **error) {
    gchar *version;
    faacEncGetVersion (&version, NULL);
    g_debug ("FAAC Encoder Module - Using FAAC %s", version);

	encoder_faac_data *efd = g_new (encoder_faac_data, 1);
	if (!efd) {
		g_set_error_literal (error, GRIP_FAACENC_ERROR, GRIP_FAACENC_ERROR_NOMEM, _("Out of memory"));
		return NULL;
	}

	efd -> codec = faacEncOpen (44100, 2, &(efd -> input_samples), & (efd -> outbufsize));
	if (!efd -> codec) {
		g_set_error_literal (error, GRIP_FAACENC_ERROR, GRIP_FAACENC_ERROR_INIT, _("Unable to init FAAC encoder"));
		g_free (efd);
		return NULL;
	}

	faacEncConfigurationPtr conf = faacEncGetCurrentConfiguration (efd -> codec);
	conf -> mpegVersion = MPEG4;
	conf -> aacObjectType = MAIN;
//    conf -> useTns = 0;
//    conf -> allowMidside = 1;
//    conf -> useLfe = 1;
//    conf -> bitRate = 24;
//    conf -> quantqual = 80;
//    conf -> outputFormat = 1;
	conf -> inputFormat = FAAC_INPUT_16BIT;
	if (!faacEncSetConfiguration (efd -> codec, conf)) {
        g_set_error_literal (error, GRIP_FAACENC_ERROR, GRIP_FAACENC_ERROR_INVALIDFORMAT, _("Invalid encoder parameters"));
        faacEncClose (efd -> codec);
        g_free (efd);
        return NULL;
	}

	// Prepare output file
	efd -> fout = fopen (filename, "wb");
	if (!efd -> fout) {
		g_set_error_literal (error, GRIP_FAACENC_ERROR, GRIP_FAACENC_ERROR_OUTFILE, _("Unable to open output file"));
		faacEncClose (efd -> codec);
		g_free (efd);
		return NULL;
	}

	// Prepare output buffer
	// Here we multiply input_samples by 4 because every sample is 16-bit and we need to hold at least two samples set
	efd -> outbuf = g_new (guint8, efd -> input_samples * 4);
	if (!efd -> outbuf) {
		g_set_error_literal (error, GRIP_FAACENC_ERROR, GRIP_FAACENC_ERROR_NOMEM, _("Out of memory"));
		faacEncClose (efd -> codec);
		fclose (efd -> fout);
		g_free (efd);
		return NULL;
	}
	efd -> buffer_avail = 0;

	return efd;
}

/* Here we use poor man's ring buffer because FaacEncEncode() must be fed
 * exactly input_samples 16-bit samples at a time, and cdparanoia is providing
 * us a different number.. */
gboolean encoder_faac_callback (gint16 *buffer, gsize bufsize, gpointer user_data) {
	gint outsize;
	encoder_faac_data *efd = (encoder_faac_data *) user_data;
	g_assert (efd);

    // Append to buffer
    g_assert (efd -> buffer_avail + bufsize <= efd -> input_samples * 4);
    memcpy (efd -> outbuf + efd -> buffer_avail, buffer, bufsize);
    efd -> buffer_avail += bufsize;
//    g_debug ("Appended %Zu bytes to buffer, now size is %Zu", bufsize, efd -> buffer_avail);

    // Consume till we can!
    while (efd -> buffer_avail >= efd -> input_samples * 2) {
//        g_debug ("Consuming %lu 16-bit samples", efd -> input_samples);

        outsize = faacEncEncode (efd -> codec, (gint32 *) efd -> outbuf, efd -> input_samples, efd -> outbuf, efd -> outbufsize);
        size_t n = fwrite (efd -> outbuf, sizeof (guint8), outsize, efd -> fout);
        if (n != outsize) {
            g_warning (_("Unable to write to output file"));
            return FALSE;
        }

        // Discard used bytes
        memmove (efd -> outbuf, efd -> outbuf + efd -> input_samples * 2, efd -> buffer_avail - efd -> input_samples * 2);
        efd -> buffer_avail -= efd -> input_samples * 2;
//        g_debug ("efd -> buffer_avail = %Zu", efd -> buffer_avail);
    }

	return TRUE;
}

gboolean encoder_faac_close (gpointer user_data, GError **error) {
	gint outsize;
	encoder_faac_data *efd = (encoder_faac_data *) user_data;
	g_assert (efd);

//    g_debug ("encoder_faac_close()");

    // Consume remaining buffer and flush FAAC output
    do {
        gulong m = MIN (efd -> input_samples, efd -> buffer_avail / 2);

//        g_debug ("Consuming %lu 16-bit samples", m);

        outsize = faacEncEncode (efd -> codec, (gint32 *) efd -> outbuf, m, efd -> outbuf, efd -> outbufsize);
        size_t n = fwrite (efd -> outbuf, sizeof (guint8), outsize, efd -> fout);
        if (n != outsize) {
            g_warning (_("Unable to write to output file"));
            return FALSE;
        }

        // Discard used bytes
        memmove (efd -> outbuf, efd -> outbuf + efd -> input_samples * 2, efd -> buffer_avail - m * 2);
        efd -> buffer_avail -= m * 2;
//        g_debug ("efd -> buffer_avail = %Zu", efd -> buffer_avail);
    } while (outsize != 0);

	fclose (efd -> fout);
	faacEncClose (efd -> codec);

	g_free (efd -> outbuf);
	g_free (efd);

	return TRUE;
}

// Encoder registration
supported_encoder faac_encoder = {
    "FAAC Encoder",
    faac_formats,
    encoder_faac_init,
    encoder_faac_close,
    encoder_faac_callback
};
