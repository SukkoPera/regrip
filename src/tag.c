/* id3.c
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
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <tag_c.h>


// GError stuff
#define GRIP_TAG_ERROR grip_tag_error_quark ()

GQuark grip_tag_error_quark (void) {
    return g_quark_from_static_string ("grip-tag-error-quark");
}

enum GripTagError {
    GRIP_TAG_ERROR_CANTOPEN,
    GRIP_TAG_ERROR_SAVEFAILED,
};


gboolean tag_file (char *filename, char *title, char *artist, char *album,
                   int year, char *comment, char *genre, unsigned
                   char tracknum, char *id3v2_encoding, GError **error) {

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    gboolean ret;
    TagLib_File *file;
    TagLib_Tag *tag;

    g_debug ("Tagging file '%s'", filename);

    taglib_set_strings_unicode (FALSE);

    file = taglib_file_new (filename);
    if (file == NULL) {
        g_set_error (error, GRIP_TAG_ERROR, GRIP_TAG_ERROR_CANTOPEN, _("Unable to open file for tagging: '%s'"), filename);
        ret = FALSE;
    } else {
        tag = taglib_file_tag (file);
        taglib_tag_set_title (tag, title);
        taglib_tag_set_artist (tag, artist);
        taglib_tag_set_album(tag, album);
        taglib_tag_set_year (tag, year);
        taglib_tag_set_comment (tag, comment);
        taglib_tag_set_genre (tag, genre);
        taglib_tag_set_track (tag, (unsigned int) tracknum);


        ret = (gboolean) taglib_file_save (file);
        if (!ret) {
            g_set_error (error, GRIP_TAG_ERROR, GRIP_TAG_ERROR_SAVEFAILED, _("Unable to save file after tagging: '%s'"), filename);
        }
        taglib_tag_free_strings ();
        taglib_file_free (file);
    }

    return ret;
}
