#include <gtk/gtk.h>
#include "window_title.h"

void update_window_title(GtkWindow* window, const char* filepath) {
    /**
     * Updates window title with current filename.
     * If filepath is NULL, shows "Untitled - Text Editor".
     */
    if (!window) return;

    char* title;
    if (filepath && g_file_test(filepath, G_FILE_TEST_EXISTS)) {
        char* basename = g_path_get_basename(filepath);
        title = g_strdup_printf("%s - Text Editor", basename);
        g_free(basename);
    } else {
        title = g_strdup("Untitled - Text Editor");
    }

    gtk_window_set_title(window, title);
    g_free(title);
}

static char *current_filename = NULL; // Tracks the current opened/saved file
