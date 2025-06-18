#include <gtk/gtk.h>
#include "gui.h"
#include "piecetable.h"
#include "undo_redo.h"
#include "search.h"
#include "window_title.h"
#include "matching.h"
#include "text_color.h"

// Forward declaration
static void on_color_menu_activate(GtkMenuItem *item, gpointer user_data);

int main(int argc, char *argv[])
{
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *file_menu, *file_item;
    GtkWidget *edit_menu, *edit_item;
    GtkWidget *view_menu, *view_item;
    GtkWidget *format_menu, *format_item; // Add format menu
    GtkWidget *new_item, *open_item, *save_item, *quit_item;
    GtkWidget *undo_item, *redo_item;
    GtkWidget *zoom_in_item, *zoom_out_item;
    GtkWidget *scrolled_window;
    GtkTextBuffer *buffer;

    gtk_init(&argc, &argv);

    undo_stack = undo_redo_stack_create();
    doc_piecetable = piecetable_create("");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simple GTK Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    update_window_title(GTK_WINDOW(window), NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menubar = gtk_menu_bar_new();

    // --- File menu ---
    file_menu = gtk_menu_new();
    file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);

    new_item = gtk_menu_item_new_with_label("New");
    open_item = gtk_menu_item_new_with_label("Open");
    save_item = gtk_menu_item_new_with_label("Save");
    quit_item = gtk_menu_item_new_with_label("Quit");

    g_signal_connect(new_item, "activate", G_CALLBACK(on_new), window);
    g_signal_connect(open_item, "activate", G_CALLBACK(on_open), window);
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save), window);
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

    // --- Edit menu ---
    edit_menu = gtk_menu_new();
    edit_item = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);

    undo_item = gtk_menu_item_new_with_label("Undo");
    redo_item = gtk_menu_item_new_with_label("Redo");
    GtkWidget *color_item = gtk_menu_item_new_with_label("Set Text Color");
    g_signal_connect(undo_item, "activate", G_CALLBACK(on_undo), NULL);
    g_signal_connect(redo_item, "activate", G_CALLBACK(on_redo), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), undo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), redo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), color_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_item);

    // --- Format menu (NEW - for fonts) ---
    format_menu = gtk_menu_new();
    format_item = gtk_menu_item_new_with_label("Format");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(format_item), format_menu);

    // Font dialog
    GtkWidget *font_dialog_item = gtk_menu_item_new_with_label("Font...");
    g_signal_connect(font_dialog_item, "activate", G_CALLBACK(show_font_dialog), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), font_dialog_item);

    // Separator
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), separator1);

    // Font family submenu
    GtkWidget *font_family_item = gtk_menu_item_new_with_label("Font Family");
    GtkWidget *font_family_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(font_family_item), font_family_menu);

    GtkWidget *mono_item = gtk_menu_item_new_with_label("Monospace");
    GtkWidget *serif_item = gtk_menu_item_new_with_label("Serif");
    GtkWidget *sans_item = gtk_menu_item_new_with_label("Sans Serif");
    GtkWidget *courier_item = gtk_menu_item_new_with_label("Courier New");
    GtkWidget *times_item = gtk_menu_item_new_with_label("Times New Roman");
    GtkWidget *arial_item = gtk_menu_item_new_with_label("Arial");

    g_signal_connect(mono_item, "activate", G_CALLBACK(on_font_monospace), NULL);
    g_signal_connect(serif_item, "activate", G_CALLBACK(on_font_serif), NULL);
    g_signal_connect(sans_item, "activate", G_CALLBACK(on_font_sans_serif), NULL);
    g_signal_connect(courier_item, "activate", G_CALLBACK(on_font_courier), NULL);
    g_signal_connect(times_item, "activate", G_CALLBACK(on_font_times), NULL);
    g_signal_connect(arial_item, "activate", G_CALLBACK(on_font_arial), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), mono_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), serif_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), sans_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), courier_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), times_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_family_menu), arial_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), font_family_item);

    // Font size submenu
    GtkWidget *font_size_item = gtk_menu_item_new_with_label("Font Size");
    GtkWidget *font_size_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(font_size_item), font_size_menu);

    GtkWidget *size8_item = gtk_menu_item_new_with_label("8");
    GtkWidget *size10_item = gtk_menu_item_new_with_label("10");
    GtkWidget *size12_item = gtk_menu_item_new_with_label("12");
    GtkWidget *size14_item = gtk_menu_item_new_with_label("14");
    GtkWidget *size16_item = gtk_menu_item_new_with_label("16");
    GtkWidget *size18_item = gtk_menu_item_new_with_label("18");
    GtkWidget *size24_item = gtk_menu_item_new_with_label("24");
    GtkWidget *size36_item = gtk_menu_item_new_with_label("36");

    g_signal_connect(size8_item, "activate", G_CALLBACK(on_font_size_8), NULL);
    g_signal_connect(size10_item, "activate", G_CALLBACK(on_font_size_10), NULL);
    g_signal_connect(size12_item, "activate", G_CALLBACK(on_font_size_12), NULL);
    g_signal_connect(size14_item, "activate", G_CALLBACK(on_font_size_14), NULL);
    g_signal_connect(size16_item, "activate", G_CALLBACK(on_font_size_16), NULL);
    g_signal_connect(size18_item, "activate", G_CALLBACK(on_font_size_18), NULL);
    g_signal_connect(size24_item, "activate", G_CALLBACK(on_font_size_24), NULL);
    g_signal_connect(size36_item, "activate", G_CALLBACK(on_font_size_36), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size8_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size10_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size12_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size14_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size16_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size18_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size24_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(font_size_menu), size36_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), font_size_item);

    // Separator
    GtkWidget *separator2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), separator2);

    // Font style toggles
    GtkWidget *bold_item = gtk_menu_item_new_with_label("Toggle Bold");
    GtkWidget *italic_item = gtk_menu_item_new_with_label("Toggle Italic");

    g_signal_connect(bold_item, "activate", G_CALLBACK(on_toggle_bold), NULL);
    g_signal_connect(italic_item, "activate", G_CALLBACK(on_toggle_italic), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), bold_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), italic_item);

    // Separator
    GtkWidget *separator3 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), separator3);

    // --- Selection formatting submenu ---
    GtkWidget *selection_item = gtk_menu_item_new_with_label("Format Selection");
    GtkWidget *selection_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(selection_item), selection_menu);

    // Selection font family
    GtkWidget *sel_font_family_item = gtk_menu_item_new_with_label("Font Family");
    GtkWidget *sel_font_family_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sel_font_family_item), sel_font_family_menu);

    GtkWidget *sel_mono_item = gtk_menu_item_new_with_label("Monospace");
    GtkWidget *sel_serif_item = gtk_menu_item_new_with_label("Serif");
    GtkWidget *sel_sans_item = gtk_menu_item_new_with_label("Sans Serif");
    GtkWidget *sel_courier_item = gtk_menu_item_new_with_label("Courier New");
    GtkWidget *sel_times_item = gtk_menu_item_new_with_label("Times New Roman");
    GtkWidget *sel_arial_item = gtk_menu_item_new_with_label("Arial");

    g_signal_connect(sel_mono_item, "activate", G_CALLBACK(on_selection_font_monospace), NULL);
    g_signal_connect(sel_serif_item, "activate", G_CALLBACK(on_selection_font_serif), NULL);
    g_signal_connect(sel_sans_item, "activate", G_CALLBACK(on_selection_font_sans_serif), NULL);
    g_signal_connect(sel_courier_item, "activate", G_CALLBACK(on_selection_font_courier), NULL);
    g_signal_connect(sel_times_item, "activate", G_CALLBACK(on_selection_font_times), NULL);
    g_signal_connect(sel_arial_item, "activate", G_CALLBACK(on_selection_font_arial), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_mono_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_serif_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_sans_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_courier_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_times_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_family_menu), sel_arial_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_font_family_item);

    // Selection font size
    GtkWidget *sel_font_size_item = gtk_menu_item_new_with_label("Font Size");
    GtkWidget *sel_font_size_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sel_font_size_item), sel_font_size_menu);

    GtkWidget *sel_size8_item = gtk_menu_item_new_with_label("8");
    GtkWidget *sel_size10_item = gtk_menu_item_new_with_label("10");
    GtkWidget *sel_size12_item = gtk_menu_item_new_with_label("12");
    GtkWidget *sel_size14_item = gtk_menu_item_new_with_label("14");
    GtkWidget *sel_size16_item = gtk_menu_item_new_with_label("16");
    GtkWidget *sel_size18_item = gtk_menu_item_new_with_label("18");
    GtkWidget *sel_size24_item = gtk_menu_item_new_with_label("24");
    GtkWidget *sel_size36_item = gtk_menu_item_new_with_label("36");

    g_signal_connect(sel_size8_item, "activate", G_CALLBACK(on_selection_font_size_8), NULL);
    g_signal_connect(sel_size10_item, "activate", G_CALLBACK(on_selection_font_size_10), NULL);
    g_signal_connect(sel_size12_item, "activate", G_CALLBACK(on_selection_font_size_12), NULL);
    g_signal_connect(sel_size14_item, "activate", G_CALLBACK(on_selection_font_size_14), NULL);
    g_signal_connect(sel_size16_item, "activate", G_CALLBACK(on_selection_font_size_16), NULL);
    g_signal_connect(sel_size18_item, "activate", G_CALLBACK(on_selection_font_size_18), NULL);
    g_signal_connect(sel_size24_item, "activate", G_CALLBACK(on_selection_font_size_24), NULL);
    g_signal_connect(sel_size36_item, "activate", G_CALLBACK(on_selection_font_size_36), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size8_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size10_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size12_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size14_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size16_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size18_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size24_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(sel_font_size_menu), sel_size36_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_font_size_item);

    // Selection style
    GtkWidget *sel_bold_item = gtk_menu_item_new_with_label("Toggle Bold");
    GtkWidget *sel_italic_item = gtk_menu_item_new_with_label("Toggle Italic");
    GtkWidget *sel_color_item = gtk_menu_item_new_with_label("Text Color...");
    GtkWidget *sel_bg_color_item = gtk_menu_item_new_with_label("Background Color...");
    GtkWidget *clear_format_item = gtk_menu_item_new_with_label("Clear Formatting");

    g_signal_connect(sel_bold_item, "activate", G_CALLBACK(on_selection_bold), NULL);
    g_signal_connect(sel_italic_item, "activate", G_CALLBACK(on_selection_italic), NULL);
    g_signal_connect(sel_color_item, "activate", G_CALLBACK(on_selection_color_dialog), window);
    g_signal_connect(sel_bg_color_item, "activate", G_CALLBACK(on_selection_background_dialog), window);
    g_signal_connect(clear_format_item, "activate", G_CALLBACK(on_clear_formatting), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_bold_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_italic_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_color_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), sel_bg_color_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(selection_menu), clear_format_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), selection_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), format_item);

    // --- View menu (Zoom In/Out) ---
    view_menu = gtk_menu_new();
    view_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);

    zoom_in_item = gtk_menu_item_new_with_label("Zoom In");
    zoom_out_item = gtk_menu_item_new_with_label("Zoom Out");

    g_signal_connect(zoom_in_item, "activate", G_CALLBACK(on_zoom_in), NULL);
    g_signal_connect(zoom_out_item, "activate", G_CALLBACK(on_zoom_out), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_in_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), zoom_out_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);

    // --- Search menu ---
    GtkWidget *search_menu, *search_item, *search_toggle_item;
    search_menu = gtk_menu_new();
    search_item = gtk_menu_item_new_with_label("Search");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(search_item), search_menu);

    search_toggle_item = gtk_menu_item_new_with_label("Find...");
    g_signal_connect(search_toggle_item, "activate", G_CALLBACK(show_search_bar), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), search_toggle_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), search_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // --- Search bar ---
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
    gtk_search_bar_set_show_close_button(GTK_SEARCH_BAR(search_bar), TRUE);
    g_signal_connect(search_bar, "close", G_CALLBACK(on_search_bar_close), NULL);

    replace_entry = gtk_entry_new();
    GtkWidget *replace_button = gtk_button_new_with_label("Replace");
    GtkWidget *replace_all_button = gtk_button_new_with_label("Replace All");

    gtk_box_pack_start(GTK_BOX(search_box), replace_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), replace_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), replace_all_button, FALSE, FALSE, 0);

    g_signal_connect(replace_button, "clicked", G_CALLBACK(on_replace_clicked), replace_entry);
    g_signal_connect(replace_all_button, "clicked", G_CALLBACK(on_replace_all_clicked), replace_entry);

    // --- Text view and scroll ---
    text_view = gtk_text_view_new();
    gtk_widget_set_name(text_view, "editor_view");
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_text_view_key_press), NULL);
    g_signal_connect(buffer, "begin-user-action", G_CALLBACK(on_begin_user_action), NULL);
    g_signal_connect(buffer, "end-user-action", G_CALLBACK(on_end_user_action), NULL);
    g_signal_connect(buffer, "changed", G_CALLBACK(on_buffer_changed), NULL);
    g_signal_connect(color_item, "activate", G_CALLBACK(on_color_menu_activate), text_view);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // --- Bracket matching ---
    init_bracket_matching(GTK_TEXT_VIEW(text_view));

    // --- Tags for bracket highlighting (example) ---
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag1 = gtk_text_buffer_create_tag(buffer, "bracket_level_1", "foreground", "blue", NULL);
    GtkTextTag *tag2 = gtk_text_buffer_create_tag(buffer, "bracket_level_2", "foreground", "green", NULL);
    GtkTextTag *tag3 = gtk_text_buffer_create_tag(buffer, "bracket_level_3", "foreground", "orange", NULL);

    // --- Initialize font system AFTER text_view is created ---
    initialize_font_system();

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

static void on_color_menu_activate(GtkMenuItem *item, gpointer user_data)
{
    GtkTextView *text_view = GTK_TEXT_VIEW(user_data);
    prompt_and_apply_color(text_view);
}