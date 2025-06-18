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
extern GtkWidget *replace_entry;

gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
void show_search_bar(GtkWidget *widget, gpointer data);
void on_search_text_changed(GtkEntry *entry, gpointer user_data);
void on_next_match(GtkWidget *widget, gpointer data);
void on_previous_match(GtkWidget *widget, gpointer data);
void on_replace_clicked(GtkWidget *widget, gpointer user_data);
void on_replace_all_clicked(GtkWidget *widget, gpointer user_data);

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
void apply_font_size_to_selection(GtkTextBuffer *buffer, int font_size);

//adding fonts
void initialize_font_system(void);
void apply_current_font(void);

// font-desc
void show_font_dialog(GtkWidget *widget, gpointer window);
void on_font_monospace(GtkWidget *widget, gpointer data);
void on_font_serif(GtkWidget *widget, gpointer data);
void on_font_sans_serif(GtkWidget *widget, gpointer data);
void on_font_courier(GtkWidget *widget, gpointer data);
void on_font_times(GtkWidget *widget, gpointer data);
void on_font_arial(GtkWidget *widget, gpointer data);
void on_toggle_bold(GtkWidget *widget, gpointer data);
void on_toggle_italic(GtkWidget *widget, gpointer data);
void on_font_size_8(GtkWidget *widget, gpointer data);
void on_font_size_10(GtkWidget *widget, gpointer data);
void on_font_size_12(GtkWidget *widget, gpointer data);
void on_font_size_14(GtkWidget *widget, gpointer data);
void on_font_size_16(GtkWidget *widget, gpointer data);
void on_font_size_18(GtkWidget *widget, gpointer data);
void on_font_size_24(GtkWidget *widget, gpointer data);
void on_font_size_36(GtkWidget *widget, gpointer data);


// Add these declarations to your gui.h file

// Individual text formatting functions
void apply_formatting_to_selection(const char *tag_name, const char *property, const char *value);
void apply_font_family_to_selection(const char *family);
void apply_font_size_to_selection_new(int size);
void apply_bold_to_selection(void);
void apply_italic_to_selection(void);
void apply_color_to_selection(const char *color);
void apply_background_to_selection(const char *color);
void clear_formatting_from_selection(void);

// Selection formatting callbacks
void on_selection_font_monospace(GtkWidget *widget, gpointer data);
void on_selection_font_serif(GtkWidget *widget, gpointer data);
void on_selection_font_sans_serif(GtkWidget *widget, gpointer data);
void on_selection_font_courier(GtkWidget *widget, gpointer data);
void on_selection_font_times(GtkWidget *widget, gpointer data);
void on_selection_font_arial(GtkWidget *widget, gpointer data);
void on_selection_bold(GtkWidget *widget, gpointer data);
void on_selection_italic(GtkWidget *widget, gpointer data);
void on_selection_font_size_8(GtkWidget *widget, gpointer data);
void on_selection_font_size_10(GtkWidget *widget, gpointer data);
void on_selection_font_size_12(GtkWidget *widget, gpointer data);
void on_selection_font_size_14(GtkWidget *widget, gpointer data);
void on_selection_font_size_16(GtkWidget *widget, gpointer data);
void on_selection_font_size_18(GtkWidget *widget, gpointer data);
void on_selection_font_size_24(GtkWidget *widget, gpointer data);
void on_selection_font_size_36(GtkWidget *widget, gpointer data);
void on_clear_formatting(GtkWidget *widget, gpointer data);
void on_selection_color_dialog(GtkWidget *widget, gpointer window);
void on_selection_background_dialog(GtkWidget *widget, gpointer window);

#endif /* GUI_H */
