#include <gmodule.h>
#include "encoder.h"
#include <config.h>

GList *encoder_modules;

static gboolean load_module (gchar *modfile) {
	g_debug ("Loading module: %s", modfile);

	/* We want lazy symbol resolving.
	 * This means we only want a symbol to be resolved if we request it. */
	GModule *module = g_module_open (modfile, G_MODULE_BIND_LAZY);
	if (!module) {
		g_error ("Unable to load module");
		return FALSE;
	}

	/* Load the symbol and assign it to our function pointer.
	 * Check for errors */
    supported_encoder *encoder;
	if (g_module_symbol (module, "encoder", (gpointer *) &encoder) == FALSE) {
		g_error ("Unable to get symbol reference: %s", g_module_error ());
		return FALSE;
	} else {
	    encoder_modules = g_list_append (encoder_modules, encoder);
	}

	// FIXME: g_module_make_resident () as it exports static symbols ???

	return TRUE;
}

gboolean load_encoder_modules (void) {
    g_debug ("Loading encoder modules");

    // Init encoder module list
    encoder_modules = NULL;

	/* Check whether glib is compiled with module support */
	if (!g_module_supported ()) {
		g_error ("Modules not supported :(");
		return FALSE;
	}

	GError *error = NULL;
	GDir *dir = g_dir_open (MODULE_DIR, 0, &error);
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
