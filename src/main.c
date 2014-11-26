/* main.c
 *
 * Copyright (c) 1998-2004  Mike Oliphant <grip@nostatic.org>
 *
 *   http://sourceforge.net/projects/grip/
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

#include <config.h>
#include <popt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "grip.h"

static gint TimeOut(gpointer data);

gboolean do_debug=TRUE;
GtkWidget* grip_app;

/* popt table */
static char *geometry=NULL;
static char *config_filename=NULL;
static char *device=NULL;
static char *scsi_device=NULL;
static int force_small=FALSE;
static int local_mode=FALSE;
static int no_redirect=FALSE;
static int verbose=FALSE;

struct poptOption options[] = {
  {
    "geometry",
    '\0',
    POPT_ARG_STRING,
    &geometry,
    0,
    N_("Specify the geometry of the main window"),
    N_("GEOMETRY")
  },
  {
    "config",
    '\0',
    POPT_ARG_STRING,
    &config_filename,
    0,
    N_("Specify the config file to use (in your home dir)"),
    N_("CONFIG")
  },
  {
    "device",
    '\0',
    POPT_ARG_STRING,
    &device,
    0,
    N_("Specify the cdrom device to use"),
    N_("DEVICE")
  },
  {
    "scsi-device",
    '\0',
    POPT_ARG_STRING,
    &scsi_device,
    0,
    N_("Specify the generic scsi device to use"),
    N_("DEVICE")
  },
  {
    "small",
    '\0',
    POPT_ARG_NONE,
    &force_small,
    0,
    N_("Launch in \"small\" (cd-only) mode"),
    NULL
  },
  {
    "local",
    '\0',
    POPT_ARG_NONE,
    &local_mode,
    0,
    N_("\"Local\" mode -- do not look up disc info on the net"),
    NULL
  },
  {
    "no-redirect",
    '\0',
    POPT_ARG_NONE,
    &no_redirect,
    0,
    N_("Do not do I/O redirection"),
    NULL
  },
  {
    "verbose",
    '\0',
    POPT_ARG_NONE,
    &verbose,
    0,
    N_("Run in verbose (debug) mode"),
    NULL
  },
  {
    NULL,
    '\0',
    0,
    NULL,
    0,
    NULL,
    NULL
  }
};

int Cmain (int argc, char* argv[]) {

  /* Unbuffer stdout */
  setvbuf(stdout, 0, _IONBF, 0);

  /* setup locale, i18n */
  gtk_set_locale();
  bindtextdomain(GETTEXT_PACKAGE,GNOMELOCALEDIR);
  textdomain(GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF8");

  do_debug=verbose;

  if(scsi_device) printf("scsi=[%s]\n",scsi_device);

  /* Start a new Grip app */
  grip_app=GripNew(geometry,device,scsi_device,config_filename,
		   force_small,local_mode,
		   no_redirect);

  gtk_widget_show(grip_app);

  gtk_timeout_add(1000,TimeOut,0);

  gtk_main();

  return 0;
}

static gint TimeOut(gpointer data)
{
  GripUpdate(grip_app);

  return TRUE;
}
