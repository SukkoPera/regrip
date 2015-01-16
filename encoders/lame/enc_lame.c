#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <lame/lame.h>
#include <encoder.h>
#include <config.h>

// GError stuff
#define GRIP_LAMEENC_ERROR lame_encoder_error_quark ()

static GQuark lame_encoder_error_quark (void) {
    return g_quark_from_static_string ("grip-lameenc-error-quark");
}

enum GripLAMEEncError {
    GRIP_LAMEENC_ERROR_NOMEM,
    GRIP_LAMEENC_ERROR_INIT,
    GRIP_LAMEENC_ERROR_INVALIDFORMAT,
    GRIP_LAMEENC_ERROR_BUFFERTOOSMALL,
    GRIP_LAMEENC_ERROR_NOTINITED,
    GRIP_LAMEENC_ERROR_PSYCHOACOUSTICPROBLEMS,
    GRIP_LAMEENC_ERROR_ENCODINGFAILED,
};


typedef struct {
	FILE *fout;
	lame_t codec;
	gulong outbufsize;
	guint8 *outbuf;
} encoder_lame_data;

#define MAX_STRING 128

#define LOG_DOMAIN "enc_lame"

static void lame_error (const char *msg, va_list args) {
	g_logv (LOG_DOMAIN, G_LOG_LEVEL_WARNING, msg, args);
}

static void lame_debug (const char *msg, va_list args) {
	g_logv (LOG_DOMAIN, G_LOG_LEVEL_DEBUG, msg, args);
}

static void lame_msg (const char *msg, va_list args) {
	g_logv (LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, msg, args);
}

typedef struct {
	GtkBuilder *builder;
	GSettings *settings;
} encoder_handle;

typedef struct {
	FILE *fout;
	lame_t codec;
	gulong outbufsize;
	guint8 *outbuf;
} file_handle;


#define REGRIP_GSCHEMA_BASE "net.sukkology.software.regrip"
#define REGRIP_ENCODER_GSCHEMA_BASE REGRIP_GSCHEMA_BASE ".encoder"
#define ENCODER_SCHEMA REGRIP_ENCODER_GSCHEMA_BASE ".lame"

G_MODULE_EXPORT gchar *enc_get_name (void) {
	return "LAME Encoder Module";
}

G_MODULE_EXPORT gchar *enc_get_version (void) {
    static gchar version[MAX_STRING] = "";

	if (version[0] == '\0') {
		snprintf (version, MAX_STRING, "LAME Encoder Module - Using LAME %s", get_lame_version ());
	}

	return version;
}

G_MODULE_EXPORT gpointer enc_init (GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	encoder_handle *handle = NULL;

#ifdef MAINTAINER_MODE
    GSettingsSchemaSource *schema_source = g_settings_schema_source_new_from_directory (GSCHEMA_DIR, g_settings_schema_source_get_default (), FALSE, error);
    if (schema_source != NULL) {
//		GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, ENCODER_SCHEMA, TRUE);
//		g_assert (schema);

		handle = g_new0 (encoder_handle, 1);

//		handle -> settings = g_settings_new_full (schema, NULL, NULL);
//		g_assert (handle -> settings);
#else
//		handle -> settings = g_settings_new (ENCODER_SCHEMA);
#endif

		// Load UI
		handle -> builder = gtk_builder_new ();
//		gtk_builder_add_from_file (handle -> builder, UI_FILE, NULL);
		//gtk_builder_connect_signals (handle -> builder, NULL);
	}

	return handle;
}

G_MODULE_EXPORT gboolean enc_close (gpointer enc_data, GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return TRUE;
}

G_MODULE_EXPORT supported_format *enc_get_formats (void) {
	static supported_format fmts[] = {
		{"MP3", "mp3", NULL},
		{NULL}
	};

	return fmts;
}

G_MODULE_EXPORT gboolean enc_start_batch (gpointer enc_data, supported_format *fmt, GError **error) {
    return TRUE;
}

G_MODULE_EXPORT gboolean enc_finish_batch (gpointer enc_data, GError **error) {
    return TRUE;
}

G_MODULE_EXPORT GtkDialog *enc_about (gpointer enc_data) {
	encoder_handle *data = (encoder_handle *) enc_data;

	GtkAboutDialog *about = GTK_ABOUT_DIALOG (gtk_builder_get_object (data -> builder, "dialog_about"));
    g_assert (about);

    // This must be done manually, since version is set in CMake
    //gtk_about_dialog_set_version (about, VERSION);

    return GTK_DIALOG (about);
}

G_MODULE_EXPORT GtkWidget *enc_get_prefs_widget (gpointer enc_data) {
	encoder_handle *data = (encoder_handle *) enc_data;

	return NULL;
}

G_MODULE_EXPORT gpointer enc_start (gpointer enc_data, supported_format *fmt, FILE *fp, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    g_return_val_if_fail (enc_data != NULL, FALSE);

    g_debug ("LAME Encoder Module - Using LAME %s", get_lame_version ());

	encoder_lame_data *eld = g_new0 (encoder_lame_data, 1);
	if (!eld) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_NOMEM, _("Out of memory"));
		return NULL;
	}

	eld -> codec = lame_init ();
	if (!eld -> codec) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_INIT, _("Unable to init LAME encoder"));
		g_free (eld);
		return NULL;
	}

	// Set message handlers
	lame_set_errorf (eld -> codec, lame_error);
	lame_set_debugf (eld -> codec, lame_debug);
	lame_set_msgf (eld -> codec, lame_msg);

	/* The default (if you set nothing) is a J-Stereo, 44.1khz
	 * 128kbps CBR mp3 file at quality 5. Override various default settings
	 * as necessary.
	 */
	lame_set_in_samplerate (eld -> codec, CD_SAMPLE_RATE);
	lame_set_num_channels (eld -> codec, CD_CHANNELS);
    lame_set_VBR (eld -> codec, vbr_default);
//	lame_set_brate (eld -> codec, 128);
//	lame_set_mode (eld -> codec, 1);
//	lame_set_quality (eld -> codec, 2);   /* 2=high  5 = medium  7=low */
	if (lame_init_params (eld -> codec) < 0) {
        g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_INVALIDFORMAT, _("Invalid encoder parameters"));
        lame_close (eld -> codec);
        g_free (eld);
        return NULL;
	}

	// Prepare output file
	g_assert (fp);
	eld -> fout = fp;

	/* Prepare output buffer: mp3buffer_size can be computed from num_samples,
	 * samplerate and encoding rate, but here is a worst case estimate:
	 *
	 * mp3buffer_size (in bytes) = 1.25 * num_samples + 7200
	 *
	 * num_samples = the number of PCM samples in *each* channel. It is
	 * not the sum of the number of samples in the L and R channels.
	 */
	eld -> outbuf = g_new (guint8, 1.25 * CD_SECTOR_SIZE / CD_CHANNELS / CD_SAMPLE_SIZE + 7200);
	if (!eld -> outbuf) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_NOMEM, _("Out of memory"));
		lame_close (eld -> codec);
		fclose (eld -> fout);
		g_free (eld);
		return NULL;
	}

	return eld;
}

G_MODULE_EXPORT gboolean enc_callback (gpointer enc_data, gpointer file_data, gint16 *buffer, gsize bufsize, GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	encoder_lame_data *eld = (encoder_lame_data *) file_data;
	g_assert (eld);

	/* num_samples = number of samples in the L (or R) channel, not the total
	 * number of samples in pcm[]
	 */
	int outsize = lame_encode_buffer_interleaved (eld -> codec, /*(short int *) */buffer, bufsize / CD_CHANNELS / CD_SAMPLE_SIZE, eld -> outbuf, eld -> outbufsize);
	if (outsize > 0) {
		// Encoder produced some data, write to output file
		size_t n = fwrite (eld -> outbuf, sizeof (guint8), outsize, eld -> fout);
		if (n != outsize) {
			g_warning (_("Unable to write to output file"));
			return FALSE;
		}
	} else if (outsize == -1) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_BUFFERTOOSMALL, _("mp3buf was too small"));
		return FALSE;
	} else if (outsize == -2) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_NOMEM, _("Out of memory"));
		return FALSE;
	} else if (outsize == -3) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_NOTINITED, _("lame_init_params() not called"));
		return FALSE;
	} else if (outsize == -4) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_PSYCHOACOUSTICPROBLEMS, _("Psycho-acoustic problems"));
		return FALSE;
	} else if (outsize < 0) {
		// Just to be sure
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_ENCODINGFAILED, _("Unknown encoding error"));
		return FALSE;
	}
	// If 0, fine, nothing to do :)

	return TRUE;
}

G_MODULE_EXPORT gboolean enc_finish (gpointer enc_data, gpointer file_data, GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (enc_data != NULL && file_data != NULL, FALSE);

	gboolean ret = TRUE;

	encoder_lame_data *eld = (encoder_lame_data *) file_data;
	g_assert (eld);

    g_debug ("encoder_lame_close()");

    // Flush LAME output
	// FIXME: Test lame_encode_flush_nogap
	int outsize;
	while ((outsize = lame_encode_flush (eld -> codec, eld -> outbuf, eld -> outbufsize)) > 0) {
        size_t n = fwrite (eld -> outbuf, sizeof (guint8), outsize, eld -> fout);
        if (n != outsize) {
            g_warning (_("Unable to write to output file"));
            ret = FALSE;
        }
	}

	//fclose (eld -> fout);
	ret = lame_close (eld -> codec) && ret;

	g_free (eld -> outbuf);
	g_free (eld);

	return ret;
}
