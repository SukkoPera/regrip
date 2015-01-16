#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>

/** Number of channels in Audio CD data */
#define CD_CHANNELS 2

/** Sample frequency for Audio CD data */
#define CD_SAMPLE_RATE 44100

/** Sample size for Audio CD data (i.e.: 16 bits) */
#define CD_SAMPLE_SIZE 2

/** Size of data we get provided at each callback, in bytes. Thus,
 * the number of 16-bit samples is CD_SECTOR_SIZE / 2. Channels are
 * interleaved, thus there are CD_SECTOR_SIZE / 4 16-bit samples for
 * each channel.
 */
#define CD_SECTOR_SIZE 2352


typedef struct {
    gchar *name;        // Human-readable name, e.g.: MP3, OGG Vorbis, etc.
    char *extension;    // File extension
    gpointer data;      // Encoder-private data
} supported_format;

/* Return value must not be freed by caller */
typedef gchar *(*encoder_get_name) (void);

/* Return value must not be freed by caller */
typedef gchar *(*encoder_get_version) (void);

/* Called once at encoder registration. Any data it returns will be the "encoder handle" passed to all subsequent calls. */
typedef gpointer (*encoder_init) (GError **error);

/* Called once at encoder deregistration */
typedef gboolean (*encoder_close) (gpointer enc_data, GError **error);

typedef supported_format *(*encoder_get_formats) (void);

typedef gboolean (*encoder_start_batch) (gpointer enc_data, supported_format *fmt, GError **error);

typedef gboolean (*encoder_finish_batch) (gpointer enc_data, GError **error);

typedef GtkDialog *(*encoder_about) (gpointer enc_data);

typedef GtkWidget *(*encoder_get_prefs_widget) (gpointer enc_data);

/* fp is already open for writing. This avoids having to care about temporary files, filename encoding, files already existing, etc.
 * Encoder must NOT close fp!
 */
typedef gpointer (*encoder_start) (gpointer enc_data, supported_format *fmt, FILE *fp, GError **error);

typedef gboolean (*encoder_callback) (gpointer enc_data, gpointer file_data, gint16 *buffer, gsize bufsize, GError **error);

typedef gboolean (*encoder_finish) (gpointer enc_data, gpointer file_data, GError **error);


typedef struct {
    encoder_get_name get_name;
    encoder_get_version get_version;
    encoder_init init;
    encoder_close close;
    encoder_get_formats get_formats;
    encoder_start_batch start_batch;
    encoder_finish_batch finish_batch;
    encoder_about about;
    encoder_get_prefs_widget get_prefs_widget;
    encoder_start start;
    encoder_callback callback;
    encoder_finish finish;

    gpointer handle;
} supported_encoder;


/* List of encoder modules */
extern GList *encoder_modules;

gboolean load_encoder_modules (GError **error);
gboolean unload_encoder_modules (void);

#endif // ENCODER_H_INCLUDED
