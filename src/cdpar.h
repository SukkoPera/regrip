/* cdpar.h
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

#ifndef CDPAR_H_INCLUDED
#define CDPAR_H_INCLUDED

#include <glib.h>

typedef gboolean (*cdrip_callback) (gint16 *buffer, gsize bufsize, gfloat progress, int smilie_idx, gpointer user_data);

gboolean rip_start (GripInfo *ginfo, cdrip_callback callback, gpointer callback_data, GError **error);

const char * const get_smilie (int slevel);

#endif // CDPAR_H_INCLUDED
