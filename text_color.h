#ifndef TEXT_COLOR_H
#define TEXT_COLOR_H

#include <gtk/gtk.h>

extern GtkWidget *global_text_view;

void prompt_and_apply_color(GtkTextView *text_view);
void on_color_response(GtkDialog *dialog, gint response_id, gpointer user_data);

#endif