#include <gtk/gtk.h>
#include "gui.h"
#include "piecetable.h"
#include "undo_redo.h"
#include "search.h"
#include "window_title.h" 
#include "matching.h"

int main(int argc, char *argv[]) {
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *file_menu, *file_item;
    GtkWidget *edit_menu, *edit_item;
    GtkWidget *new_item, *open_item, *save_item, *quit_item;
    GtkWidget *undo_item, *redo_item;
    GtkWidget *scrolled_window;
    GtkTextBuffer *buffer;

    gtk_init(&argc, &argv);

    undo_stack = undo_redo_stack_create();
    doc_piecetable = piecetable_create("");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simple GTK Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    update_window_title(GTK_WINDOW(window), NULL); ///   PLACE for window title
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menubar = gtk_menu_bar_new();

    // File menu
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

    // Edit menu
    edit_menu = gtk_menu_new();
    edit_item = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);

    undo_item = gtk_menu_item_new_with_label("Undo");
    redo_item = gtk_menu_item_new_with_label("Redo");
    g_signal_connect(undo_item, "activate", G_CALLBACK(on_undo), NULL);
    g_signal_connect(redo_item, "activate", G_CALLBACK(on_redo), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), undo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), redo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Search menu
    GtkWidget *search_menu, *search_item;
    GtkWidget *search_toggle_item;

    search_menu = gtk_menu_new();
    search_item = gtk_menu_item_new_with_label("Search");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(search_item), search_menu);

    search_toggle_item = gtk_menu_item_new_with_label("Find...");
    g_signal_connect(search_toggle_item, "activate", G_CALLBACK(show_search_bar), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), search_toggle_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), search_item);

    // Search bar
    search_bar = gtk_search_bar_new();
    search_entry = gtk_search_entry_new(); 
    GtkWidget *search_prev = gtk_button_new_with_label("Previous");
    GtkWidget *search_next = gtk_button_new_with_label("Next");

    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(search_box), search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), search_prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), search_next, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(search_bar), search_box);

    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_text_changed), NULL);
    g_signal_connect(search_prev, "clicked", G_CALLBACK(on_previous_match), NULL);
    g_signal_connect(search_next, "clicked", G_CALLBACK(on_next_match), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), search_bar, FALSE, FALSE, 0);

    text_view = gtk_text_view_new();
    gtk_widget_set_name(text_view, "editor_view");
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_text_view_key_press), NULL);

    g_signal_connect(buffer, "begin-user-action", G_CALLBACK(on_begin_user_action), NULL);
    g_signal_connect(buffer, "end-user-action", G_CALLBACK(on_end_user_action), NULL);
    g_signal_connect(buffer, "changed", G_CALLBACK(on_buffer_changed), NULL);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    //bracket matching
    init_bracket_matching(GTK_TEXT_VIEW(text_view));
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);

    GtkTextTag *tag1 = gtk_text_buffer_create_tag(buffer, "bracket_level_1", "foreground", "blue", NULL);
    GtkTextTag *tag2 = gtk_text_buffer_create_tag(buffer, "bracket_level_2", "foreground", "green", NULL);
    GtkTextTag *tag3 = gtk_text_buffer_create_tag(buffer, "bracket_level_3", "foreground", "orange", NULL);



    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
