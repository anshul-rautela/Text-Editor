#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "piecetable.h"
#include "undo_redo.h"
#include "search.h"
extern GtkWidget *text_view;
extern Piecetable doc_piecetable;
extern UndoRedoStack *undo_stack;
extern GtkWidget *search_bar;
extern GtkWidget *search_entry;
gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
void show_search_bar(GtkWidget *widget, gpointer data);
void on_search_text_changed(GtkEntry *entry, gpointer user_data);
void on_next_match(GtkWidget *widget, gpointer data);
void on_previous_match(GtkWidget *widget, gpointer data);
void update_piece_table_from_buffer(void);
void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);
void on_new(GtkWidget *widget, gpointer data);
void on_open(GtkWidget *widget, gpointer window);
void on_save(GtkWidget *widget, gpointer window);
void on_quit(GtkWidget *widget, gpointer data);
void on_search_bar_close(GtkSearchBar *search_bar, gpointer user_data);
// Zoom operations
void on_zoom_in(GtkWidget *widget, gpointer data);
void on_zoom_out(GtkWidget *widget, gpointer data);
void initialize_textview_font_size(void);

// Edit operations
void on_undo(GtkWidget *widget, gpointer data);
void on_redo(GtkWidget *widget, gpointer data);

// Text buffer callbacks
void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);
void on_begin_user_action(GtkTextBuffer *buffer, gpointer user_data);
void on_end_user_action(GtkTextBuffer *buffer, gpointer user_data);

// Key handling
gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

// Piece table operations
void update_piece_table_from_buffer(void);

// Parenthesis matching
void setup_parenthesis_matching(GtkWidget *text_view, GtkTextBuffer *buffer);
void on_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer data);
void find_matching_opening_bracket(GtkTextBuffer *buffer, GtkTextIter *close_pos, char close_char);
void find_matching_closing_bracket(GtkTextBuffer *buffer, GtkTextIter *open_pos, char open_char);

#endif /* GUI_H */
