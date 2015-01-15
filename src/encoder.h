#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <stdio.h>
#include <glib.h>

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


/* fp is already open for writing. This avoids having to care about temporary files, filename encoding, files already existing, etc.
 * Encoder must NOT close fp!
 */
typedef gpointer (*encoder_init) (supported_format *fmt, FILE *fp, gpointer user_data, GError **error);
typedef gboolean (*encoder_close) (gpointer encoder_data, GError **error);
typedef gboolean (*encoder_callback) (gint16 *buffer, gsize bufsize, gpointer user_data);


typedef struct {
	gchar *name;    // "sndfile encoder Version x.y.z"
    supported_format *supported_formats;        // NULL-terminated array
    encoder_init init;
    encoder_close close;
    encoder_callback callback;
} supported_encoder;

//extern supported_encoder *supported_encoders[];
extern GList *encoder_modules;

gboolean load_encoder_modules (void);
gboolean unload_encoder_modules (void);

#endif // ENCODER_H_INCLUDED
