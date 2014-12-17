#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <glib.h>

//typedef struct {
////    file_format format;
//
//    /* WAV: Ignored
//     * VORBIS: Quality
//     * FLAC: Compression
//     * MP3: Bitrate
//     */
//    int quality;
//} encoder_options;


typedef struct {
    gchar *name;    // Human-readable name, e.g.: MP3, OGG Vorbis, etc.
    gpointer data;
//	char *extension;
} supported_format;


// fmt is encoder_data above
typedef gpointer (*encoder_init) (gpointer *fmt, gchar *filename, gpointer user_data, GError **error);
typedef gboolean (*encoder_close) (gpointer encoder_data, GError **error);
typedef gboolean (*encoder_callback) (gint16 *buffer, gsize bufsize, gpointer user_data);


typedef struct {
	gchar *name;    // "sndfile encoder Version x.y.z"
    supported_format *supported_formats;        // NULL-terminated array
    encoder_init init;
    encoder_close close;
    encoder_callback callback;
} supported_encoder;

//extern supported_encoder supported_encoders[];
extern supported_encoder *supported_encoders[];

#endif // ENCODER_H_INCLUDED
