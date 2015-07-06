/* discdb.c
 *
 * Based on code from libcdaudio 0.5.0 (Copyright (C)1998 Tony Arcieri)
 *
 * Copyright (c) 1998-2015 Mike Oliphant <contact@nostatic.org>
 * Copyright (c) 2014-2015 SukkoPera <software@sukkology.net>
 *
 *   https://github.com/SukkoPera/regrip
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <cddb/cddb.h>
#include "discdb.h"


// GError stuff
#define GRIP_DISCDB_ERROR discdb_error_quark ()

GQuark discdb_error_quark (void) {
	return g_quark_from_static_string ("grip-discdb-error-quark");
}

enum GripDiscDbError {
    GRIP_DISCDB_ERROR_NOMEM,
    GRIP_DISCDB_ERROR_QUERYFAILED,
    GRIP_DISCDB_ERROR_READFAILED
};


#define smart_strncpy(dst, src, n) \
    if (src) { \
            strncpy (dst, src, n); \
    }

// Gets a single CDDB entry
DiscData *cddb_get_entry (DiscDBServer *server, gboolean local_only, gchar *category, guint id, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	libcddb_init ();
	libcddb_set_flags (CDDB_F_NO_TRACK_ARTIST);

	cddb_conn_t *conn = cddb_new ();
	if (conn == NULL) {
		g_set_error_literal (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_NOMEM, _("Out of memory, unable to create CDDB connection"));
		return NULL;
	}

	// Use specified server, in case
	if (server) {
    //	cddb_set_server_name (conn, server -> name);
        if (server -> port != 0)
            cddb_set_server_port (conn, server -> port);
	}

	// Only use cache, if requested
	if (local_only)
        cddb_cache_only (conn);

	cddb_disc_t *disc = cddb_disc_new ();
	if (conn == NULL) {
		g_set_error_literal (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_NOMEM, _("Out of memory, unable to create disc"));
		return NULL;
	}

    cddb_disc_set_category_str (disc, category);
    cddb_disc_set_discid (disc, id);

    if (cddb_read (conn, disc)) {
        DiscData *data = g_new0 (DiscData, 1);
        g_assert (data);

        // Disc data
        data -> data_id = cddb_disc_get_discid (disc);
        smart_strncpy (data -> data_title, cddb_disc_get_title (disc), MAX_STRING);
        smart_strncpy (data -> data_artist, cddb_disc_get_artist (disc), MAX_STRING);
        smart_strncpy (data -> data_extended, cddb_disc_get_ext_data (disc), MAX_EXTENDED_STRING);
        smart_strncpy (data -> data_genre, cddb_disc_get_genre (disc), MAX_STRING);
    //            data -> category = g_strdup (cddb_disc_get_category_str(disc));
        data -> data_year = cddb_disc_get_year (disc);
    //            data -> num_tracks = cddb_disc_get_track_count(disc);
        data -> revision = cddb_disc_get_revision (disc);

        // Track data
        int i;
        cddb_track_t *track = cddb_disc_get_track_first (disc);
        for (i = 0; track != NULL && i < MAX_TRACKS; ++i) {
            /* ... use track ...  */
            smart_strncpy (data -> data_track[i].track_name, cddb_track_get_title (track), MAX_STRING);
            smart_strncpy (data -> data_track[i].track_artist, cddb_track_get_artist (track), MAX_STRING);
            smart_strncpy (data -> data_track[i].track_extended, cddb_track_get_ext_data (track), MAX_EXTENDED_STRING);

            track = cddb_disc_get_track_next (disc);
        }

        return data;
    } else {
        /* something went wrong, print error */
        g_set_error (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_READFAILED, _("CDDB read query results failed: %s"), cddb_error_str (cddb_errno (conn)));
        // FIXME: FREE
        return NULL;
    }

    libcddb_shutdown ();
}


// Returns a list of DiscData with no TrackData
GList *cddb_lookup (DiscDBServer *server, gboolean local_only, const DiscInfo *dinfo, GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	int i;
	GList *results = NULL;

	libcddb_init ();
	libcddb_set_flags (CDDB_F_NO_TRACK_ARTIST);

	cddb_conn_t *conn = cddb_new ();
	if (conn == NULL) {
		g_set_error_literal (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_NOMEM, _("Out of memory, unable to create CDDB connection"));
		return results;
	}

	// Use specified server, in case
	if (server) {
    //	cddb_set_server_name (conn, server -> name);
        if (server -> port != 0)
            cddb_set_server_port (conn, server -> port);
	}

	// Only use cache, if requested
	if (local_only)
        cddb_cache_only (conn);

	cddb_disc_t *disc = cddb_disc_new ();
	if (disc == NULL) {
		g_set_error_literal (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_NOMEM, _("Out of memory, unable to create disc"));
        cddb_destroy (conn);
		return results;
	}

	// Disc length in seconds
	cddb_disc_set_length (disc, (dinfo -> length).mins * 60 + (dinfo -> length).secs);

    // Tracks
	for (i = 0; i < dinfo -> num_tracks; ++i) {
		cddb_track_t *track = cddb_track_new ();
		if (track == NULL) {
			g_set_error_literal (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_NOMEM, _("Out of memory, unable to create track"));
			cddb_disc_destroy (disc);
			cddb_destroy (conn);
		}

		cddb_track_set_frame_offset (track, (dinfo -> track)[i].start_frame);
		cddb_disc_add_track (disc, track);
	}

	/* Execute query command */
	g_debug ("Querying CDDB server %s with len %u", cddb_get_server_name (conn), cddb_disc_get_length (disc));
	int matches = cddb_query (conn, disc);
	if (matches < 0) {
		/* Something went wrong */
		g_set_error (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_QUERYFAILED, _("CDDB query failed: %s"), cddb_error_str (cddb_errno (conn)));
	} else if (matches == 1) {
	    /* Single match, fetch it and return a complete record (i.e.: with tracks) */
	    g_debug ("Found single CDDB match");

        if (cddb_read (conn, disc)) {
            DiscData *data = g_new0 (DiscData, 1);
            g_assert (data);

            // Disc data
            data -> data_id = cddb_disc_get_discid (disc);
            smart_strncpy (data -> data_title, cddb_disc_get_title (disc), MAX_STRING);
            smart_strncpy (data -> data_artist, cddb_disc_get_artist (disc), MAX_STRING);
            smart_strncpy (data -> data_extended, cddb_disc_get_ext_data (disc), MAX_EXTENDED_STRING);
            smart_strncpy (data -> data_genre, cddb_disc_get_genre (disc), MAX_STRING);
            smart_strncpy (data -> data_category, cddb_disc_get_category_str (disc), MAX_STRING);
            data -> data_year = cddb_disc_get_year (disc);
//            data -> num_tracks = cddb_disc_get_track_count(disc);
            data -> revision = cddb_disc_get_revision (disc);

            // Track data
            cddb_track_t *track = cddb_disc_get_track_first (disc);
			for (i = 0; track != NULL && i < MAX_TRACKS; ++i) {
				/* ... use track ...  */
				smart_strncpy (data -> data_track[i].track_name, cddb_track_get_title (track), MAX_STRING);
				smart_strncpy (data -> data_track[i].track_artist, cddb_track_get_artist (track), MAX_STRING);
				smart_strncpy (data -> data_track[i].track_extended, cddb_track_get_ext_data (track), MAX_EXTENDED_STRING);

//				cddb_track_destroy (track);
				track = cddb_disc_get_track_next (disc);
			}

            results = g_list_append (results, data);
        } else {
            /* Something went wrong */
            g_set_error (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_READFAILED, _("CDDB read query results failed: %s"), cddb_error_str (cddb_errno (conn)));

            // results will be NULL at this point, which is fine
        }
	} else if (matches > 1) {
	    /* Multiple matches, only return details for each album (without track data).
         * Once a match is chosen, full details for it can be retrieved with the cddb_get_entry() function.
         */
		g_debug ("Found %d CDDB matches:", matches);
        do {
			g_debug ("- %s/%u: %s - %s (%s, %d)", cddb_disc_get_category_str (disc), cddb_disc_get_discid (disc), cddb_disc_get_artist (disc), cddb_disc_get_title (disc), cddb_disc_get_genre (disc), cddb_disc_get_year (disc));

            if (cddb_read (conn, disc)) {
                DiscData *data = g_new0 (DiscData, 1);
                g_assert (data);

                // Disc data
                data -> data_id = cddb_disc_get_discid (disc);
                smart_strncpy (data -> data_title, cddb_disc_get_title (disc), MAX_STRING);
                smart_strncpy (data -> data_artist, cddb_disc_get_artist (disc), MAX_STRING);
                smart_strncpy (data -> data_extended, cddb_disc_get_ext_data (disc), MAX_EXTENDED_STRING);
                smart_strncpy (data -> data_genre, cddb_disc_get_genre (disc), MAX_STRING);
                smart_strncpy (data -> data_category, cddb_disc_get_category_str (disc), MAX_STRING);
                data -> data_year = cddb_disc_get_year (disc);
    //            data -> num_tracks = cddb_disc_get_track_count(disc);
                data -> revision = cddb_disc_get_revision (disc);

                results = g_list_append (results, data);
            } else {
				/* Something went wrong */
				// Results will be partly populated at this point, and subsequent results might be read fine, so just issue a warning
				g_warning (_("CDDB read query results failed: %s"), cddb_error_str (cddb_errno (conn)));
//                g_set_error (error, GRIP_DISCDB_ERROR, GRIP_DISCDB_ERROR_READFAILED, _("CDDB read query results failed: %s"), cddb_error_str (cddb_errno (conn)));
			}
        } while (cddb_query_next (conn, disc));
	}

    cddb_disc_destroy (disc);
	cddb_destroy (conn);

	libcddb_shutdown ();

	return results;
}


#if 0

/* Convert numerical genre to text */
char *DiscDBGenre (int genre) {
	if (genre > 11) {
		return ("unknown");
	}

	return discdb_genres[genre];
}

/* Convert genre from text form into an integer value */
int discDBGenre_value (char *genre) {
	int pos;

	for (pos = 0; pos < 12; pos++)
		if (!strcmp (genre, discdb_genres[pos])) {
			return pos;
		}

	return 0;
}


/* Split string into title/artist */

void discDBParse_title (char *buf, char *title, char *artist, char *sep) {
	char *tmp;

	tmp = strtok (buf, sep);

	if (!tmp) {
		return;
	}

	g_snprintf (artist, 256, "%s", g_strstrip (tmp));

	tmp = strtok (NULL, "");

	if (tmp) {
		g_snprintf (title, 256, "%s", g_strstrip (tmp));
	} else {
		strcpy (title, artist);
	}
}

static char *StrConvertEncoding (char *str, const char *from, const char *to,
                                 int max_len) {
	char *conv_str;
	gsize rb, wb;

	if (!str) {
		return NULL;
	}

	conv_str = g_convert_with_fallback (str, strlen (str), to, from, NULL, &rb, &wb,
	                                    NULL);

	if (!conv_str) {
		return str;
	}

	g_snprintf (str, max_len, "%s", conv_str);

	g_free (conv_str);

	return str;
}

gboolean discDBUTF8_validate (const DiscInfo *disc, const DiscData *data) {
	int track;

	if (data->data_title && !g_utf8_validate (data->data_title, -1, NULL)) {
		return FALSE;
	}

	if (data->data_artist && !g_utf8_validate (data->data_artist, -1, NULL)) {
		return FALSE;
	}

	if (data->data_extended && !g_utf8_validate (data->data_extended, -1, NULL)) {
		return FALSE;
	}

	for (track = 0; track < disc->num_tracks; track++) {
		if (data->data_track[track].track_name
		        && !g_utf8_validate (data->data_track[track].track_name, -1, NULL)) {
			return FALSE;
		}

		if (data->data_track[track].track_artist
		        && !g_utf8_validate (data->data_track[track].track_artist, -1, NULL)) {
			return FALSE;
		}

		if (data->data_track[track].track_extended
		        && !g_utf8_validate (data->data_track[track].track_extended, -1, NULL)) {
			return FALSE;
		}
	}

	return TRUE;
}


static void discDBConvert_encoding (DiscInfo *disc, DiscData *data,
                                   const char *from, const char *to) {
	int track;

	StrConvertEncoding (data->data_title, from, to, 256);
	StrConvertEncoding (data->data_artist, from, to, 256);
	StrConvertEncoding (data->data_extended, from, to, 4096);

	for (track = 0; track < disc->num_tracks; track++) {
		StrConvertEncoding (data->data_track[track].track_name, from, to, 256);
		StrConvertEncoding (data->data_track[track].track_artist, from, to, 256);
		StrConvertEncoding (data->data_track[track].track_extended, from, to, 4096);
	}
}

/* Read the actual DiscDB entry */

gboolean discDBRead (DiscInfo *disc, DiscDBServer *server,
                     DiscDBHello *hello, DiscDBEntry *entry,
                     DiscData *data, char *encoding) {
	int index;
	GString *cmd;
	char *result, *inbuffer, *dataptr;

	if (!disc->have_info) {
		CDStat (disc, TRUE);
	}

	// FIXME
//  data->data_genre=entry->entry_genre;
	data->data_id = DiscDBDiscid (disc);
	* (data->data_extended) = '\0';
	* (data->data_title) = '\0';
	* (data->data_artist) = '\0';
	* (data->data_playlist) = '\0';
	data->data_multi_artist = FALSE;
	data->data_year = 0;
	data->revision = -1;

	for (index = 0; index < MAX_TRACKS; index++) {
		* (data->data_track[index].track_name) = '\0';
		* (data->data_track[index].track_artist) = '\0';
		* (data->data_track[index].track_extended) = '\0';
	}

	cmd = g_string_new (NULL);

	g_string_sprintf (cmd, "cddb+read+%s+%08x", DiscDBGenre (entry->entry_genre),
	                  entry->entry_id);

	result = DiscDBMakeRequest (server, hello, cmd->str);

	g_string_free (cmd, TRUE);

	if (!result) {
		return FALSE;
	}

	dataptr = result;

	inbuffer = discDBReadLine (&dataptr);

	while ((inbuffer = discDBReadLine (&dataptr))) {
		DiscDBProcessLine (inbuffer, data, disc->num_tracks);
	}

	/* Both disc title and artist have been stuffed in the title field, so the
	   need to be separated */

	discDBParse_title (data->data_title, data->data_title, data->data_artist, "/");

	free (result);

	/* Don't allow the genre to be overwritten */
//  data->data_genre=entry->entry_genre;    FIXME

	if (strcasecmp (encoding, "utf-8")) {
		discDBConvert_encoding (disc, data, encoding, "utf-8");
	}

	return TRUE;
}

#endif // 0
