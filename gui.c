#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include <gdk/gdkkeysyms.h>
GtkWidget *text_view = NULL;
Piecetable doc_piecetable = NULL;
UndoRedoStack *undo_stack = NULL;
static char *prev_text = NULL;

void update_piece_table_from_buffer(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (doc_piecetable != NULL) {
        piecetable_free(doc_piecetable);
    }
    doc_piecetable = piecetable_create(text);
    g_free(text);
}
void on_begin_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    prev_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void on_end_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *new_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (prev_text) {
        undo_redo_push(undo_stack, prev_text, new_text);
        g_free(prev_text);
        prev_text = NULL;
    }
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

void on_new(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
    if (doc_piecetable != NULL) {
        piecetable_free(doc_piecetable);
    }
    doc_piecetable = piecetable_create("");
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
            if (doc_piecetable != NULL) {
                piecetable_free(doc_piecetable);
            }
            doc_piecetable = piecetable_create(contents);
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
        }
        g_free(text);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

void on_quit(GtkWidget *widget, gpointer data) {
    if (doc_piecetable != NULL) {
        piecetable_free(doc_piecetable);
    }
    if (undo_stack != NULL) {
        undo_redo_stack_free(undo_stack);
    }
    gtk_main_quit();
}
gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter cursor_iter;
    gchar open_char = 0;
    gchar close_char = 0;

    // Auto-close brackets
    switch (event->keyval) {
        case GDK_KEY_parenleft:    // (
            open_char = '('; close_char = ')'; break;
        case GDK_KEY_bracketleft:  // [
            open_char = '['; close_char = ']'; break;
        case GDK_KEY_braceleft:    // {
            open_char = '{'; close_char = '}'; break;
        case GDK_KEY_less:         // <
            open_char = '<'; close_char = '>'; break;
        default:
            break; // Continue checking for shortcuts
    }

    if (open_char != 0 && close_char != 0) {
        // Insert bracket pair
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter,
                                         gtk_text_buffer_get_insert(buffer));
        gchar text[3] = {open_char, close_char, '\0'};
        gtk_text_buffer_insert(buffer, &cursor_iter, text, 2);

        // Move cursor between brackets
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter,
                                         gtk_text_buffer_get_insert(buffer));
        gtk_text_iter_backward_char(&cursor_iter);
        gtk_text_buffer_place_cursor(buffer, &cursor_iter);
        return TRUE;  // Skip default handling
    }

    // Ctrl+Z for Undo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_z)) {
        on_undo(NULL, NULL);
        return TRUE;
    }

    // Ctrl+Y for Redo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_y)) {
        on_redo(NULL, NULL);
        return TRUE;
    }

    return FALSE; // Let normal keys pass through
}
