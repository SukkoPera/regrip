#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <lame.h>
#include <encoder.h>
#include <config.h>

static supported_format lame_formats[] = {
    {"MP3", "mp3", NULL},
    {NULL}
};


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

/*

typedef struct {
	GtkBuilder *builder;
	GSettings *settings;
} encoder_private;

typedef struct {
	FILE *fout;
	lame_t codec;
	gulong outbufsize;
	guint8 *outbuf;
} encoder_file_private;

#define REGRIP_GSCHEMA_BASE "net.sukkology.software.regrip"
#define REGRIP_ENCODER_GSCHEMA_BASE (REGRIP_GSCHEMA_BASE ".encoder")
#define ENCODER_SCHEMA (REGRIP_ENCODER_GSCHEMA_BASE ".lame")

G_MODULE_EXPORT gpointer encoder_init (GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	
#ifdef MAINTAINER_MODE
    GSettingsSchemaSource *schema_source = g_settings_schema_source_new_from_directory (GSCHEMA_DIR, g_settings_schema_source_get_default (), FALSE, error);
    if (schema_source != NULL) {
		GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, ENCODER_GSETTINGS_SCHEMA, TRUE);
		g_assert (schema);
		
		encoder_private *data = g_new0 (encoder_private, 1);
		
		data -> settings = g_settings_new_full (schema, NULL, NULL);
		g_assert (data -> settings);
#else
		data -> settings = g_settings_new (ENCODER_GSETTINGS_SCHEMA);
#endif

		// Load UI
		data -> builder = gtk_builder_new ();
		gtk_builder_add_from_file (data -> builder, UI_FILE, NULL);
		//gtk_builder_connect_signals (data -> builder, NULL);
	
		ret = data;
	} else {
		ret = NULL;
	}
	
	return ret;
}

G_MODULE_EXPORT gboolean encoder_close (gpointer enc_data, GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	return TRUE;
}

G_MODULE_EXPORT gchar *encoder_get_version (void) {
    static gchar version[MAX_STRING] = "";
	
	if (version[0] == '\0') {
		char lamever[MAX_STRING];
		get_lame_version (lamever, MAX_STRING, NULL);
		snprintf (version, MAX_STRING, "LAME Encoder Module - Using LAME %s", lamever);
	}

	return version;
}

G_MODULE_EXPORT supported_format *get_formats (void) {
	static supported_format fmts[] = {
		{"MP3", "mp3", NULL},
		{NULL}
	};
	
	return fmts;
}

G_MODULE_EXPORT gboolean encoder_start_batch (gpointer enc_data, supported_format *fmt, GError **error) {
}

G_MODULE_EXPORT gboolean encoder_finish_batch (gpointer enc_data, GError **error) {
}


G_MODULE_EXPORT GtkDialog *encoder_about (gpointer enc_data) {
	encoder_private *data = (encoder_private *) enc_data;

	GtkAboutDialog *about = GTK_ABOUT_DIALOG (gtk_builder_get_object (data -> builder, "dialog_about"));
    g_assert (about);

    // This must be done manually, since version is set in CMake
    //gtk_about_dialog_set_version (about, VERSION);
}

G_MODULE_EXPORT GtkWidget *encoder_get_prefs_widget (gpointer enc_data) {
	encoder_private *data = (encoder_private *) enc_data;
	
	return NULL;
}

*/

//G_MODULE_EXPORT gpointer encoder_start (gpointer enc_data, supported_format *fmt, FILE *fp, GError **error) {
G_MODULE_EXPORT gpointer encoder_lame_init (supported_format *fmt, FILE *fp, gpointer user_data, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    gchar version[MAX_STRING];
	get_lame_version (version, MAX_STRING, NULL);
    g_debug ("LAME Encoder Module - Using LAME %s", version);

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
/*	lame_set_brate (eld -> codec, 128);
	lame_set_mode (eld -> codec, 1);
	lame_set_quality (eld -> codec, 2);   /* 2=high  5 = medium  7=low */
	if (lame_init_params (eld -> codec) <= 0) {
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
	eld -> outbuf = g_new (guint8, 1.25 * num_samples + 7200);
	if (!eld -> outbuf) {
		g_set_error_literal (error, GRIP_LAMEENC_ERROR, GRIP_LAMEENC_ERROR_NOMEM, _("Out of memory"));
		lame_close (eld -> codec);
		fclose (eld -> fout);
		g_free (eld);
		return NULL;
	}

	return eld;
}

//G_MODULE_EXPORT gboolean encoder_callback (gpointer enc_data, gpointer file_data, gint16 *buffer, gsize bufsize, GError **error) {
G_MODULE_EXPORT gboolean encoder_lame_callback (gint16 *buffer, gsize bufsize, gpointer user_data /*, GError **error */) {
	//g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	encoder_lame_data *eld = (encoder_lame_data *) user_data;
	g_assert (eld);
	
	// FIXME
	GError **error = NULL;

	/* num_samples = number of samples in the L (or R) channel, not the total
	 * number of samples in pcm[]
	 */
	int outsize = lame_encode_buffer_interleaved (eld -> codec, /*(short int *) */eld -> outbuf, eld -> input_samples / CD_CHANNELS / CD_SAMPLE_SIZE, eld -> outbuf, eld -> outbufsize);
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

//G_MODULE_EXPORT gboolean encoder_finish (gpointer enc_data, gpointer file_data, GError **error) {
G_MODULE_EXPORT gboolean encoder_lame_close (gpointer user_data, GError **error) {
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	//g_return_val_if_fail (enc_data != NULL && file_data != NULL, FALSE);
	
	gboolean ret = TRUE;
	
	encoder_lame_data *eld = (encoder_lame_data *) user_data;
	g_assert (eld);

//    g_debug ("encoder_lame_close()");

    // Flush LAME output
	// FIXME: Test lame_encode_flush_nogap
	gint outsize = lame_encode_flush (eld -> codec, eld -> outbuf, eld -> outbufsize);
	size_t n = fwrite (eld -> outbuf, sizeof (guint8), outsize, eld -> fout);
	if (n != outsize) {
		g_warning (_("Unable to write to output file"));
		ret = FALSE;
	}

	//fclose (eld -> fout);
	ret = lame_close (eld -> codec) && ret;

	g_free (eld -> outbuf);
	g_free (eld);

	return ret;
}

// Encoder registration
G_MODULE_EXPORT supported_encoder encoder = {
    "LAME Encoder",
    lame_formats,
    encoder_lame_init,
    encoder_lame_close,
    encoder_lame_callback
};
