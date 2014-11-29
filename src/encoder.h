#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <glib.h>

typedef enum {
    FILEFMT_WAV,
    FILEFMT_VORBIS,
    FILEFMT_FLAC,
    FILEFMT_MP3
} file_format;

typedef struct {
    file_format format;

    /* WAV: Ignored
     * VORBIS: Quality
     * FLAC: Compression
     * MP3: Bitrate
     */
    int quality;
} encoder_options;

gboolean encoder_callback (gint16 *buffer, gsize bufsize, gpointer user_data);
gpointer encoder_init (gchar *filename, encoder_options *opts, GError **error);
gboolean encoder_close (gpointer encoder_data, GError **error);

#endif // ENCODER_H_INCLUDED
