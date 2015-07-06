/* dialog.c
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "dialog.h"

void show_warning (GtkWidget *parentWin, char *text) {
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (parentWin),
	                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s",
	                    text);
	gtk_window_set_title (GTK_WINDOW (dialog), _ ("Warning"));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void show_error (GtkWidget *parentWin, char *text) {
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (parentWin),
	                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
	                    text);
	gtk_window_set_title (GTK_WINDOW (dialog), _ ("Error"));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void change_str_val (GtkWidget *widget, gpointer data) {
	strcpy ((char *) data, gtk_entry_get_text (GTK_ENTRY (widget)));
}

GtkWidget *MakeStrEntry (GtkWidget **entry, char *var, char *name,
                         int len, gboolean editable) {
	GtkWidget *widget;
	GtkWidget *label;
	GtkWidget *hbox;

	hbox = gtk_hbox_new (FALSE, 5);

	label = gtk_label_new (name);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	widget = gtk_entry_new_with_max_length (len);
	gtk_entry_set_editable (GTK_ENTRY (widget), editable);

	if (var) {
		gtk_entry_set_text (GTK_ENTRY (widget), var);

		gtk_signal_connect (GTK_OBJECT (widget), "changed",
		                    GTK_SIGNAL_FUNC (change_str_val), (gpointer) var);
	}

	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	gtk_entry_set_position (GTK_ENTRY (widget), 0);

	gtk_widget_show (widget);

	if (entry) {
		*entry = widget;
	}

	return hbox;
}

static void on_folder_selected (GtkFileChooserButton *widget, gpointer user_data) {
    strcpy ((char *) user_data, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget)));
    g_debug ("Output folder is now: '%s'", (char *) user_data);
}

GtkWidget *MakeFolderSelector (GtkWidget **entry, char *var, char *name) {
	GtkWidget *widget;
	GtkWidget *label;
	GtkWidget *hbox;

	hbox = gtk_hbox_new (FALSE, 5);

	label = gtk_label_new (name);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	widget = gtk_file_chooser_button_new (name, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	if (var) {
		g_signal_connect (G_OBJECT (widget), "selection-changed",
		                    G_CALLBACK (on_folder_selected), (gpointer) var);
	}

	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	if (entry) {
		*entry = widget;
	}

	return hbox;
}

void change_int_val (GtkWidget *widget, gpointer data) {
	*((int *)data) = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

GtkWidget *MakeNumEntry (GtkWidget **entry, int *var, char *name, int len) {
	GtkWidget *widget;
	char buf[80];
	GtkWidget *label;
	GtkWidget *hbox;

	hbox = gtk_hbox_new (FALSE, 5);

	label = gtk_label_new (name);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	widget = gtk_entry_new_with_max_length (len);
	gtk_entry_set_alignment (GTK_ENTRY (widget), 1);        // Right-align
//	gtk_widget_set_usize (widget, len * 8 + 5, 0);

	if (var) {
		sprintf (buf, "%d", *var);
		gtk_entry_set_text (GTK_ENTRY (widget), buf);
		gtk_signal_connect (GTK_OBJECT (widget), "changed",
		                    GTK_SIGNAL_FUNC (change_int_val), (gpointer)var);
	}

	gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	if (entry) {
		*entry = widget;
	}

	return hbox;
}

void change_bool_val (GtkWidget *widget, gpointer data) {
	* ((gboolean *)data) = !* ((gboolean *)data);
}

GtkWidget *MakeCheckButton (GtkWidget **button, gboolean *var, char *name) {
	GtkWidget *widget;

	widget = gtk_check_button_new_with_label (name);

	if (var) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
		                              *var);
		gtk_signal_connect (GTK_OBJECT (widget), "clicked",
		                    GTK_SIGNAL_FUNC (change_bool_val),
		                    (gpointer)var);
	}

	if (button) {
		*button = widget;
	}

	return widget;
}
