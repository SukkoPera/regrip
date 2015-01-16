/* discdb.h
 *
 * Based on code from libcdaudio 0.5.0 (Copyright (C)1998 Tony Arcieri)
 *
 * All changes Copyright (c) 1998-2002  Mike Oliphant <oliphant@gtk.org>
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

#ifndef GRIP_DISCDB_H
#define GRIP_DISCDB_H

#include <glib.h>
#include "cddev.h"
#include "common.h"

#define MAX_EXTENDED_STRING 4096


/* Track database structure */
typedef struct _track_data {
	char track_name[MAX_STRING];                 /* Track name */
	char track_artist[MAX_STRING];       /* Track artist */
	char track_extended[MAX_EXTENDED_STRING];        /* Extended information */
} TrackData;

/* Disc database structure */
typedef struct _disc_data {
    // The first two fields uniquely identify a CDDB entry
	unsigned int data_id;                 /* CD id */
	char data_category[MAX_STRING];                 /* Disc title */

	char data_title[MAX_STRING];                 /* Disc title */
	char data_artist[MAX_STRING];        /* Disc artist */
	char data_extended[MAX_EXTENDED_STRING];         /* Extended information */
	char data_genre[MAX_STRING];             /* Discdb genre */
	int data_year;                      /* Disc year */
	char data_playlist[MAX_STRING];            /* Playlist info */
	gboolean data_multi_artist;         /* Is CD multi-artist? */
	TrackData data_track[MAX_TRACKS];   /* Track names */
	int revision;                       /* Database revision */
} DiscData;


/* DiscDB server structure */
typedef struct _discdb_server {
	char name[MAX_STRING];
	char cgi_prog[MAX_STRING];
	int port;
//	int use_proxy;
//	ProxyServer *proxy;
} DiscDBServer;


DiscData *cddb_get_entry (DiscDBServer *server, gboolean local_only, gchar *category, guint id, GError **error);

// Returns a list of DiscData with no TrackData
GList *cddb_lookup (DiscDBServer *server, gboolean local_only, const DiscInfo *dinfo, GError **error);



#if 0

#include <stdio.h>


typedef struct _discdb_hello {
	/* Program */
	char hello_program[MAX_STRING];
	/* Program version */
	char hello_version[MAX_STRING];
	int proto_version;
} DiscDBHello;

/* DiscDB entry */
typedef struct _discdb_entry {
	unsigned int entry_id;
	int entry_genre;
} DiscDBEntry;

/* An entry in the query list */
struct query_list_entry {
	int list_genre;
	int list_id;
	char list_title[MAX_STRING];
	char list_artist[MAX_STRING];
};

#define MAX_INEXACT_MATCHES         16

/* DiscDB query structure */

typedef struct _discdb_query {
	int query_match;
	int query_matches;
	struct query_list_entry query_list[MAX_INEXACT_MATCHES];
} DiscDBQuery;

/* Match values returned by a query */

#define MATCH_NOMATCH    0
#define MATCH_EXACT  1
#define MATCH_INEXACT    2


unsigned int DiscDBDiscid (DiscInfo *disc);
char *DiscDBGenre (int genre);
int DiscDBGenreValue (char *genre);
gboolean DiscDBDoQuery (DiscInfo *disc, DiscDBServer *server,
                        DiscDBHello *hello, DiscDBQuery *query);
gboolean DiscDBRead (DiscInfo *disc, DiscDBServer *server,
                     DiscDBHello *hello, DiscDBEntry *entry,
                     DiscData *data, char *encoding);
gboolean DiscDBStatDiscData (DiscInfo *disc);
int DiscDBReadDiscData (DiscInfo *disc, DiscData *ddata, const char *encoding);
int DiscDBWriteDiscData (DiscInfo *disc, DiscData *ddata, FILE *outfile,
                         gboolean gripext, gboolean freedbext, char *encoding);
void DiscDBParseTitle (char *buf, char *title, char *artist, char *sep);
char *ChopWhite (char *buf);

#endif /* GRIP_DISCDB_H */

#endif // 0
