#include <gtk/gtk.h>

GtkWidget *text_view;
double font_scale = 10.0;  // Track the font scale factor
GtkCssProvider *provider = NULL;  // Keep the provider as a global

// Simple undo/redo system
#define MAX_UNDO_STEPS 100
typedef struct {
    gchar *text;
    gint cursor_position;
} TextState;

TextState undo_stack[MAX_UNDO_STEPS];
TextState redo_stack[MAX_UNDO_STEPS];
int undo_index = 0;
int redo_index = 0;
gboolean ignore_changes = FALSE;

// Function to get current cursor position
static gint get_cursor_position(GtkTextBuffer *buffer) {
    GtkTextMark *mark;
    GtkTextIter iter;
    
    mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    return gtk_text_iter_get_offset(&iter);
}

// Function to save current text state
static void save_text_state() {
    if (ignore_changes) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    
    // Clear redo stack if we're adding a new undo step
    while (redo_index > 0) {
        redo_index--;
        g_free(redo_stack[redo_index].text);
        redo_stack[redo_index].text = NULL;
    }
    
    // Free the oldest entry if we're at max capacity
    if (undo_index >= MAX_UNDO_STEPS) {
        g_free(undo_stack[0].text);
        
        // Shift all entries down
        for (int i = 0; i < MAX_UNDO_STEPS - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_index = MAX_UNDO_STEPS - 1;
    }
    
    // Get current text
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    undo_stack[undo_index].text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    undo_stack[undo_index].cursor_position = get_cursor_position(buffer);
    undo_index++;
    
    g_print("Text state saved. Undo steps: %d\n", undo_index);
}
static void update_font_size() {
    if (provider != NULL) {
        GtkStyleContext *style_context = gtk_widget_get_style_context(text_view);
        gtk_style_context_remove_provider(style_context, GTK_STYLE_PROVIDER(provider));
        g_object_unref(provider);
    }

    provider = gtk_css_provider_new();

    char css[256];
    snprintf(css, sizeof(css),
             "#editor_view { font-family: monospace; font-size: %.1fpt; }",
             12.0 * font_scale);

    GError *error = NULL;
    gtk_css_provider_load_from_data(provider, css, -1, &error);
    if (error) {
        g_printerr("Error applying CSS: %s\n", error->message);
        g_clear_error(&error);
        return;
    } else {
        g_print("Updated font scale = %.2f\n", font_scale);
    }

    GtkStyleContext *style_context = gtk_widget_get_style_context(text_view);
    gtk_style_context_add_provider(style_context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    // 🧠 This makes sure the widget visually updates
    gtk_widget_queue_draw(text_view);
}


// Set cursor position
static void set_cursor_position(GtkTextBuffer *buffer, gint position) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, position);
    gtk_text_buffer_place_cursor(buffer, &iter);
}

// Undo action
static void on_undo(GtkWidget *widget, gpointer data) {
    if (undo_index <= 0) {
        g_print("Nothing to undo\n");
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    
    // Save current state to redo stack first
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    if (redo_index < MAX_UNDO_STEPS) {
        redo_stack[redo_index].text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        redo_stack[redo_index].cursor_position = get_cursor_position(buffer);
        redo_index++;
    }
    
    // Go back one step in undo stack
    undo_index--;
    
    // Prevent this from triggering a new undo step
    ignore_changes = TRUE;
    
    // Restore text from undo stack
    gtk_text_buffer_set_text(buffer, undo_stack[undo_index].text, -1);
    
    // Restore cursor position
    set_cursor_position(buffer, undo_stack[undo_index].cursor_position);
    
    // Free the memory for this step, it's no longer needed
    g_free(undo_stack[undo_index].text);
    undo_stack[undo_index].text = NULL;
    
    ignore_changes = FALSE;
    g_print("Undo performed. Remaining undo steps: %d\n", undo_index);
}

// Redo action
static void on_redo(GtkWidget *widget, gpointer data) {
    if (redo_index <= 0) {
        g_print("Nothing to redo\n");
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    // Go back one step in redo stack
    redo_index--;
    
    // Prevent this from triggering a new undo step
    ignore_changes = TRUE;
    
    // Restore text from redo stack
    gtk_text_buffer_set_text(buffer, redo_stack[redo_index].text, -1);
    
    // Restore cursor position
    set_cursor_position(buffer, redo_stack[redo_index].cursor_position);
    
    // Save this state to the undo stack
    if (undo_index < MAX_UNDO_STEPS) {
        undo_stack[undo_index].text = g_strdup(redo_stack[redo_index].text);
        undo_stack[undo_index].cursor_position = redo_stack[redo_index].cursor_position;
        undo_index++;
    }
    
    // Free the memory for this step
    g_free(redo_stack[redo_index].text);
    redo_stack[redo_index].text = NULL;
    
    ignore_changes = FALSE;
    g_print("Redo performed\n");
}

// Handle buffer changed signal
static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    save_text_state();
}

// Handle key press events
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    // Check if Control key is pressed
    if (event->state & GDK_CONTROL_MASK) {
        // Handle zoom keys
        if (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal) {
            font_scale *= 1.2;  // Increase by 20%
            update_font_size();
            g_print("Zoom in: Scale = %.2f\n", font_scale);
            return TRUE;  // Event handled
        }
        else if (event->keyval == GDK_KEY_minus) {
            font_scale *= 0.8;  // Decrease by 20%
            if (font_scale < 0.1) font_scale = 1.0;  // Prevent too small fonts
            update_font_size();
            g_print("Zoom out: Scale = %.2f\n", font_scale);
            return TRUE;  // Event handled
        }
        else if (event->keyval == GDK_KEY_0) {
            font_scale = 10.0;  // Reset to default
            update_font_size();
            g_print("Zoom reset: Scale = %.2f\n", font_scale);
            return TRUE;  // Event handled
        }
        
        // Handle undo/redo keys
        else if (event->keyval == GDK_KEY_z) {
            on_undo(widget, NULL);
            return TRUE;
        }
        else if (event->keyval == GDK_KEY_y || 
                (event->keyval == GDK_KEY_z && (event->state & GDK_SHIFT_MASK))) {
            on_redo(widget, NULL);
            return TRUE;
        }
    }
    return FALSE;  // Event not handled
}

// Callback: New
static void on_new(GtkWidget *widget, gpointer data)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    ignore_changes = TRUE;
    gtk_text_buffer_set_text(buffer, "", -1);
    ignore_changes = FALSE;
    
    // Reset undo/redo stacks
    for (int i = 0; i < undo_index; i++) {
        g_free(undo_stack[i].text);
        undo_stack[i].text = NULL;
    }
    undo_index = 0;
    
    for (int i = 0; i < redo_index; i++) {
        g_free(redo_stack[i].text);
        redo_stack[i].text = NULL;
    }
    redo_index = 0;
    
    // Save the initial empty state
    save_text_state();
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
            
            // Don't trigger change events while loading file
            ignore_changes = TRUE;
            gtk_text_buffer_set_text(buffer, contents, -1);
            ignore_changes = FALSE;
            
            // Reset undo/redo stacks
            for (int i = 0; i < undo_index; i++) {
                g_free(undo_stack[i].text);
                undo_stack[i].text = NULL;
            }
            undo_index = 0;
            
            for (int i = 0; i < redo_index; i++) {
                g_free(redo_stack[i].text);
                redo_stack[i].text = NULL;
            }
            redo_index = 0;
            
            // Save initial file state
            save_text_state();
            
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
    if (provider != NULL) {
        g_object_unref(provider);
    }
    
    // Free undo/redo stacks
    for (int i = 0; i < undo_index; i++) {
        g_free(undo_stack[i].text);
    }
    
    for (int i = 0; i < redo_index; i++) {
        g_free(redo_stack[i].text);
    }
    
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *file_menu, *file_item;
    GtkWidget *edit_menu, *edit_item;  // New Edit menu
    GtkWidget *new_item, *open_item, *save_item, *quit_item;
    GtkWidget *undo_item, *redo_item;  // Undo/Redo menu items
    GtkWidget *scrolled_window;
    GtkTextBuffer *buffer;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Menu Bar
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

    // Edit menu with Undo/Redo
    edit_menu = gtk_menu_new();
    edit_item = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);

    undo_item = gtk_menu_item_new_with_label("Undo (Ctrl+Z)");
    redo_item = gtk_menu_item_new_with_label("Redo (Ctrl+Y)");

    g_signal_connect(undo_item, "activate", G_CALLBACK(on_undo), NULL);
    g_signal_connect(redo_item, "activate", G_CALLBACK(on_redo), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), undo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), redo_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Create text view and buffer
    text_view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    // Connect to buffer changed signal
    g_signal_connect(buffer, "changed", G_CALLBACK(on_buffer_changed), NULL);
    
    // Add text view to scrolled window
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Connect key press event to the window for global handling
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    // Initialize undo/redo stacks
    for (int i = 0; i < MAX_UNDO_STEPS; i++) {
        undo_stack[i].text = NULL;
        redo_stack[i].text = NULL;
    }
    
    // Show all widgets before applying initial font size
    gtk_widget_show_all(window);
    
    // Apply initial font size
    update_font_size();
    
    // Save initial state (empty document)
    save_text_state();
    
    // Start main loop
    gtk_main();

    return 0;
}