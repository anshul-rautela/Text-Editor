#include "text_color.h"
#include <string.h>
#include <stdlib.h>
#include "piecetable.h"

GtkWidget *global_text_view = NULL;

void prompt_and_apply_color(GtkTextView *text_view)
{
    global_text_view = GTK_WIDGET(text_view);

    GtkWidget *dialog = gtk_color_chooser_dialog_new("Choose Text Color", NULL);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    g_signal_connect(dialog, "response", G_CALLBACK(on_color_response), dialog);
    gtk_widget_show(dialog);
}

void on_color_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if (response_id == GTK_RESPONSE_OK)
    {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);

        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(global_text_view));

        GtkTextIter start, end;
        if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end))
        {
            GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
            GtkTextTag *tag = gtk_text_tag_new(NULL);

            gchar *rgba_str = gdk_rgba_to_string(&color);
            g_object_set(tag, "foreground-rgba", &color, NULL);
            gtk_text_tag_table_add(tag_table, tag);
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
            g_free(rgba_str);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}