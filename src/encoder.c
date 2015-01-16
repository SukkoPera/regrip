#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include "encoder.h"
#include <config.h>

GList *encoder_modules;

static gboolean load_module (gchar *modfile) {
    gboolean ret = TRUE;

	g_debug ("Loading module: %s", modfile);

	/* We want lazy symbol resolving.
	 * This means we only want a symbol to be resolved if we request it. */
	GModule *module = g_module_open (modfile, G_MODULE_BIND_LAZY);
	if (!module) {
		g_warning ("Unable to load module");
		ret = FALSE;
	} else {
        supported_encoder *encoder = g_new (supported_encoder, 1);

        if (g_module_symbol (module, "enc_get_name", (gpointer *) &(encoder -> get_name)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_get_version", (gpointer *) &(encoder -> get_version)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_init", (gpointer *) &(encoder -> init)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_close", (gpointer *) &(encoder -> close)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_get_formats", (gpointer *) &(encoder -> get_formats)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_start_batch", (gpointer *) &(encoder -> start_batch)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_finish_batch", (gpointer *) &(encoder -> finish_batch)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_about", (gpointer *) &(encoder -> about)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_get_prefs_widget", (gpointer *) &(encoder -> get_prefs_widget)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_start", (gpointer *) &(encoder -> start)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_callback", (gpointer *) &(encoder -> callback)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }
        if (g_module_symbol (module, "enc_finish", (gpointer *) &(encoder -> finish)) == FALSE) {
            g_warning ("Unable to get symbol reference: %s", g_module_error ());
            ret = FALSE;
        }

        if (ret) {
            g_debug ("Initializing encoder \"%s\"", encoder -> get_version ());

            GError *error = NULL;
            if ((encoder -> handle = encoder -> init (&error))) {
                encoder_modules = g_list_append (encoder_modules, encoder);
                // FIXME: g_module_make_resident () as it exports static symbols ???
            } else {
                if (error) {
                    g_warning (_("Encoder initialization failed: %s"), error -> message);
                    g_error_free (error);
                } else {
                    g_warning (_("Encoder initialization failed: %s"), _("Unknown error"));
                }
            }
        }
	}

	return ret;
}

gboolean load_encoder_modules (GError **error) {
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    g_debug ("Loading encoder modules");

    // Init encoder module list
    encoder_modules = NULL;

	/* Check whether glib is compiled with module support */
	if (!g_module_supported ()) {
		g_error ("Modules not supported :(");
		return FALSE;
	}

	GDir *dir = g_dir_open (MODULE_DIR, 0, error);
	if (dir) {
        const gchar *file;
        while ((file = g_dir_read_name (dir))) {
//            g_debug ("--> %s", file);
            gchar *fullfile = g_build_filename (MODULE_DIR, file, NULL);
            load_module (fullfile);
            g_free (fullfile);
        }

        g_dir_close (dir);
	}

	g_debug ("load_encoder_modules() finished");

	return TRUE;
}

gboolean unload_encoder_modules (void) {
#if 0
	/* We're nice citizens and close all references when we leave */
	if (g_module_close (module) == FALSE) {
		g_error ("Unable to close module: %s", g_module_error() );
		return FALSE;
	}

	return TRUE;
#endif
    return FALSE;
}
