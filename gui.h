#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "piecetable.h"
#include "undo_redo.h"

extern GtkWidget *text_view;
extern Piecetable doc_piecetable;
extern UndoRedoStack *undo_stack;
gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

void update_piece_table_from_buffer(void);
void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);
void on_new(GtkWidget *widget, gpointer data);
void on_open(GtkWidget *widget, gpointer window);
void on_save(GtkWidget *widget, gpointer window);
void on_quit(GtkWidget *widget, gpointer data);
void on_undo(GtkWidget *widget, gpointer data);
void on_redo(GtkWidget *widget, gpointer data);
void on_begin_user_action(GtkTextBuffer *buffer, gpointer user_data);
void on_end_user_action(GtkTextBuffer *buffer, gpointer user_data);
#endif // GUI_H
