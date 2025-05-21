#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "piecetable.h"
#include "search.h"
#include "undo_redo.h"
#include "window_title.h"

// --- Global Widgets and State ---

static char *current_filename = NULL; // Tracks the current opened/saved file

GtkWidget *text_view = NULL;
GtkWidget *search_bar = NULL;
GtkWidget *search_entry = NULL;

Piecetable doc_piecetable = NULL;
UndoRedoStack *undo_stack = NULL;

static char *prev_text = NULL;

// For Zoom in and Zoom out
static int current_font_size = 12; // Default font size
static GtkCssProvider *zoom_css_provider = NULL;

// Helper to apply the current font size to the text view
void apply_textview_font_size(int font_size) {
    if (!zoom_css_provider)
        zoom_css_provider = gtk_css_provider_new();

    char css[128];
    snprintf(css, sizeof(css),
             "textview { font-size: %dpt; }", font_size);

    gtk_css_provider_load_from_data(zoom_css_provider, css, -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(text_view);
    gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(zoom_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
}

// Call this in your main window setup after creating text_view:
void initialize_textview_font_size() {
    apply_textview_font_size(current_font_size);
}

// Handler for Zoom In
void on_zoom_in(GtkWidget *widget, gpointer data) {
    if (current_font_size < 48) { // Max font size
        current_font_size += 2;
        apply_textview_font_size(current_font_size);
    }
}

// Handler for Zoom Out
void on_zoom_out(GtkWidget *widget, gpointer data) {
    if (current_font_size > 6) { // Min font size
        current_font_size -= 2;
        apply_textview_font_size(current_font_size);
    }
}

// For search results navigation
static SearchResults current_results = {0};
static int current_match = -1;

// --- Utility Functions ---

void update_piece_table_from_buffer(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (doc_piecetable != NULL)
        piecetable_free(doc_piecetable);

    doc_piecetable = piecetable_create(text);
    g_free(text);
}

// --- Undo/Redo Integration ---

void on_begin_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    prev_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void on_end_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *new_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (prev_text)
        undo_redo_push(undo_stack, prev_text, new_text);

    g_free(prev_text);
    prev_text = NULL;
    g_free(new_text);
}

void on_undo(GtkWidget *widget, gpointer data) {
    const char *text = undo_redo_undo(undo_stack);
    if (text) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        g_signal_handlers_block_by_func(buffer, on_buffer_changed, NULL);
        gtk_text_buffer_set_text(buffer, text, -1);
        g_signal_handlers_unblock_by_func(buffer, on_buffer_changed, NULL);
    }
}

void on_redo(GtkWidget *widget, gpointer data) {
    const char *text = undo_redo_redo(undo_stack);
    if (text) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        g_signal_handlers_block_by_func(buffer, on_buffer_changed, NULL);
        gtk_text_buffer_set_text(buffer, text, -1);
        g_signal_handlers_unblock_by_func(buffer, on_buffer_changed, NULL);
    }
}

void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    update_piece_table_from_buffer();
}

// --- File Operations ---

void on_new(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    if (doc_piecetable != NULL)
        piecetable_free(doc_piecetable);

    doc_piecetable = piecetable_create("");

    // Free and reset filename
    if (current_filename) {
        g_free(current_filename);
        current_filename = NULL;
    }

    update_window_title(GTK_WINDOW(data), NULL); // Show "Untitled"
}

void on_open(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog;
    GtkTextBuffer *buffer;
    gchar *contents = NULL;
    gsize length;
    GError *error = NULL;

    dialog = gtk_file_chooser_dialog_new("Open File",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (g_file_get_contents(filename, &contents, &length, &error)) {
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_set_text(buffer, contents, -1);

            if (doc_piecetable != NULL)
                piecetable_free(doc_piecetable);

            doc_piecetable = piecetable_create(contents);

            // Store filename
            if (current_filename) g_free(current_filename);
            current_filename = g_strdup(filename);

            update_window_title(GTK_WINDOW(window), current_filename); // Update title

            g_free(contents);
        } else {
            g_print("Error reading file: %s\n", error->message);
            g_clear_error(&error);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_save(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    gchar *text;

    dialog = gtk_file_chooser_dialog_new("Save File",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

        if (!g_file_set_contents(filename, text, -1, NULL)) {
            g_print("Error saving file!\n");
        } else {
            // Store filename and update title
            if (current_filename) g_free(current_filename);
            current_filename = g_strdup(filename);
            update_window_title(GTK_WINDOW(window), current_filename);
        }

        g_free(text);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_quit(GtkWidget *widget, gpointer data) {
    if (doc_piecetable != NULL)
        piecetable_free(doc_piecetable);
    if (undo_stack != NULL)
        undo_redo_stack_free(undo_stack);
    gtk_main_quit();
}

// --- Search Integration ---

void show_search_bar(GtkWidget *widget, gpointer data) {
    gtk_search_bar_set_search_mode(GTK_SEARCH_BAR(search_bar), TRUE);
    gtk_widget_grab_focus(GTK_WIDGET(search_entry));
}

void on_search_text_changed(GtkEntry *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    search_results_free(&current_results);

    if (strlen(text) > 0) {
        current_results = kmp_search(text, doc_piecetable);
        current_match = current_results.count > 0 ? 0 : -1;
    } else {
        current_match = -1;
    }

    if (current_match != -1) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        int match_offset = current_results.indices[current_match];
        int match_length = strlen(gtk_entry_get_text(entry));

        GtkTextIter match_start, match_end;
        gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
        match_end = match_start;
        gtk_text_iter_forward_chars(&match_end, match_length);

        gtk_text_buffer_select_range(buffer, &match_start, &match_end);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
    }
}

void on_next_match(GtkWidget *widget, gpointer data) {
    if (current_results.count == 0) return;
    current_match = (current_match + 1) % current_results.count;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    int match_offset = current_results.indices[current_match];
    int match_length = strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)));

    GtkTextIter match_start, match_end;
    gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
    match_end = match_start;
    gtk_text_iter_forward_chars(&match_end, match_length);

    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
}

void on_previous_match(GtkWidget *widget, gpointer data) {
    if (current_results.count == 0) return;
    current_match = (current_match - 1 + current_results.count) % current_results.count;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    int match_offset = current_results.indices[current_match];
    int match_length = strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)));

    GtkTextIter match_start, match_end;
    gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
    match_end = match_start;
    gtk_text_iter_forward_chars(&match_end, match_length);

    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
}

// --- Key Press Handler (Undo/Redo, Bracket Auto-close, Search) ---

gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter cursor_iter;
    gchar open_char = 0;
    gchar close_char = 0;

    // Auto-close brackets
    switch (event->keyval) {
        case GDK_KEY_parenleft: open_char = '('; close_char = ')'; break;
        case GDK_KEY_bracketleft: open_char = '['; close_char = ']'; break;
        case GDK_KEY_braceleft: open_char = '{'; close_char = '}'; break;
        case GDK_KEY_less: open_char = '<'; close_char = '>'; break;
        default: break;
    }

    if (open_char != 0 && close_char != 0) {
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, gtk_text_buffer_get_insert(buffer));
        gchar text[3] = {open_char, close_char, '\0'};
        gtk_text_buffer_insert(buffer, &cursor_iter, text, 2);

        // Move cursor between brackets
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, gtk_text_buffer_get_insert(buffer));
        gtk_text_iter_backward_char(&cursor_iter);
        gtk_text_buffer_place_cursor(buffer, &cursor_iter);

        return TRUE; // Skip default handling
    }

    // Ctrl+F: Show search bar
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_f)) {
        show_search_bar(NULL, NULL);
        return TRUE;
    }

    // Ctrl+Z: Undo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_z)) {
        on_undo(NULL, NULL);
        return TRUE;
    }

    // Ctrl+Y: Redo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_y)) {
        on_redo(NULL, NULL);
        return TRUE;
    }

    return FALSE; // Let normal keys pass through
}
