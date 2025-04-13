#include <gtk/gtk.h>

GtkWidget *text_view;

// Callback: New
static void on_new(GtkWidget *widget, gpointer data)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

// Callback: Open
static void on_open(GtkWidget *widget, gpointer window)
{
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

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (g_file_get_contents(filename, &contents, &length, &error))
        {
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_set_text(buffer, contents, -1);
            g_free(contents);
        }
        else
        {
            g_print("Error reading file: %s\n", error->message);
            g_clear_error(&error);
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

// Callback: Save
static void on_save(GtkWidget *widget, gpointer window)
{
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

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

        if (!g_file_set_contents(filename, text, -1, NULL))
        {
            g_print("Error saving file!\n");
        }

        g_free(text);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

// Callback: Quit
static void on_quit(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *file_menu, *file_item;
    GtkWidget *new_item, *open_item, *save_item, *quit_item;
    GtkWidget *scrolled_window;

    gtk_init(&argc, &argv);

    // Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Menu
    menubar = gtk_menu_bar_new();
    file_menu = gtk_menu_new();
    file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);

    new_item = gtk_menu_item_new_with_label("New");
    open_item = gtk_menu_item_new_with_label("Open");
    save_item = gtk_menu_item_new_with_label("Save");
    quit_item = gtk_menu_item_new_with_label("Quit");

    g_signal_connect(new_item, "activate", G_CALLBACK(on_new), NULL);
    g_signal_connect(open_item, "activate", G_CALLBACK(on_open), window);
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save), window);
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Text area
    text_view = gtk_text_view_new();
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Show and run
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
