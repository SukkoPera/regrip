#ifndef TAG_H_INCLUDED
#define TAG_H_INCLUDED

#include <glib.h>

gboolean tag_file (char *filename, char *title, char *artist, char *album,
                   int year, char *comment, char *genre, unsigned
                   char tracknum, char *id3v2_encoding, GError **error);

#endif // TAG_H_INCLUDED
