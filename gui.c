#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "piecetable.h"
#include "search.h"
#include "undo_redo.h"
#include "window_title.h"

// --- Global Widgets and State ---

static char *current_filename = NULL; // Tracks the current opened/saved file

GtkWidget *text_view = NULL;
GtkWidget *search_bar = NULL;
GtkWidget *search_entry = NULL;
GtkWidget *replace_entry = NULL;
Piecetable doc_piecetable = NULL;
UndoRedoStack *undo_stack = NULL;

static char *prev_text = NULL;

// -----For Zoom in and Zoom out------
static int current_font_size = 12; // Default font size
static GtkCssProvider *zoom_css_provider = NULL;

// -----Font Management------
static char *current_font_family = NULL;
static gboolean font_bold = FALSE;
static gboolean font_italic = FALSE;
static GtkCssProvider *font_css_provider = NULL;

// Initialize default font
void initialize_font_system() {
    if (!current_font_family) {
        current_font_family = g_strdup("Monospace");
    }
    if (!font_css_provider) {
        font_css_provider = gtk_css_provider_new();
    }
    apply_current_font();
}


// Add these functions to your gui.c file

// --- Individual Text Formatting Functions ---

// Apply formatting to selected text or at cursor position
void apply_formatting_to_selection(const char *tag_name, const char *property, const char *value) {
    if (!text_view) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gboolean has_selection;
    
    // Check if there's a selection
    has_selection = gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    
    if (has_selection) {
        // Apply to selection
        GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, NULL, property, value, NULL);
        gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
    } else {
        // No selection - create a tag for future typing
        GtkTextMark *cursor = gtk_text_buffer_get_insert(buffer);
        gtk_text_buffer_get_iter_at_mark(buffer, &start, cursor);
        
        GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, NULL, property, value, NULL);
        // Store tag for current typing position
        g_object_set_data(G_OBJECT(buffer), "current_format_tag", tag);
    }
}

// Font family for selection
void apply_font_family_to_selection(const char *family) {
    apply_formatting_to_selection(NULL, "family", family);
}

// Font size for selection
void apply_font_size_to_selection_new(int size) {
    char size_str[16];
    snprintf(size_str, sizeof(size_str), "%d", size * PANGO_SCALE);
    apply_formatting_to_selection(NULL, "size", size_str);
}

// Bold for selection
void apply_bold_to_selection() {
    if (!text_view) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gboolean has_selection;
    
    has_selection = gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    
    if (has_selection) {
        // Check if selection is already bold
        GtkTextTag *bold_tag = NULL;
        
        // Look for existing bold tag
        GSList *tags = gtk_text_iter_get_tags(&start);
        for (GSList *l = tags; l != NULL; l = l->next) {
            GtkTextTag *tag = GTK_TEXT_TAG(l->data);
            gboolean is_bold;
            g_object_get(tag, "weight-set", &is_bold, NULL);
            if (is_bold) {
                PangoWeight weight;
                g_object_get(tag, "weight", &weight, NULL);
                if (weight >= PANGO_WEIGHT_BOLD) {
                    bold_tag = tag;
                    break;
                }
            }
        }
        g_slist_free(tags);
        
        if (bold_tag) {
            // Remove bold
            gtk_text_buffer_remove_tag(buffer, bold_tag, &start, &end);
        } else {
            // Add bold
            GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, NULL, 
                "weight", PANGO_WEIGHT_BOLD, NULL);
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        }
    }
}

// Italic for selection
void apply_italic_to_selection() {
    if (!text_view) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gboolean has_selection;
    
    has_selection = gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    
    if (has_selection) {
        // Check if selection is already italic
        GtkTextTag *italic_tag = NULL;
        
        GSList *tags = gtk_text_iter_get_tags(&start);
        for (GSList *l = tags; l != NULL; l = l->next) {
            GtkTextTag *tag = GTK_TEXT_TAG(l->data);
            gboolean is_italic;
            g_object_get(tag, "style-set", &is_italic, NULL);
            if (is_italic) {
                PangoStyle style;
                g_object_get(tag, "style", &style, NULL);
                if (style == PANGO_STYLE_ITALIC || style == PANGO_STYLE_OBLIQUE) {
                    italic_tag = tag;
                    break;
                }
            }
        }
        g_slist_free(tags);
        
        if (italic_tag) {
            // Remove italic
            gtk_text_buffer_remove_tag(buffer, italic_tag, &start, &end);
        } else {
            // Add italic
            GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, NULL, 
                "style", PANGO_STYLE_ITALIC, NULL);
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        }
    }
}

// Color for selection
void apply_color_to_selection(const char *color) {
    apply_formatting_to_selection(NULL, "foreground", color);
}

// Background color for selection
void apply_background_to_selection(const char *color) {
    apply_formatting_to_selection(NULL, "background", color);
}

// Remove all formatting from selection
void clear_formatting_from_selection() {
    if (!text_view) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gboolean has_selection;
    
    has_selection = gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    
    if (has_selection) {
        gtk_text_buffer_remove_all_tags(buffer, &start, &end);
    }
}

// Individual text formatting callback functions
void on_selection_font_monospace(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Monospace");
}

void on_selection_font_serif(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Serif");
}

void on_selection_font_sans_serif(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Sans");
}

void on_selection_font_courier(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Courier New");
}

void on_selection_font_times(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Times New Roman");
}

void on_selection_font_arial(GtkWidget *widget, gpointer data) {
    apply_font_family_to_selection("Arial");
}

void on_selection_bold(GtkWidget *widget, gpointer data) {
    apply_bold_to_selection();
}

void on_selection_italic(GtkWidget *widget, gpointer data) {
    apply_italic_to_selection();
}

void on_selection_font_size_8(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(8);
}

void on_selection_font_size_10(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(10);
}

void on_selection_font_size_12(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(12);
}

void on_selection_font_size_14(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(14);
}

void on_selection_font_size_16(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(16);
}

void on_selection_font_size_18(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(18);
}

void on_selection_font_size_24(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(24);
}

void on_selection_font_size_36(GtkWidget *widget, gpointer data) {
    apply_font_size_to_selection_new(36);
}

void on_clear_formatting(GtkWidget *widget, gpointer data) {
    clear_formatting_from_selection();
}

// Color selection dialog
void on_selection_color_dialog(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_color_chooser_dialog_new("Choose Text Color", GTK_WINDOW(window));
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        
        char color_str[32];
        snprintf(color_str, sizeof(color_str), "rgba(%d,%d,%d,%.2f)",
                (int)(color.red * 255),
                (int)(color.green * 255),
                (int)(color.blue * 255),
                color.alpha);
        
        apply_color_to_selection(color_str);
    }
    
    gtk_widget_destroy(dialog);
}

// Background color selection dialog
void on_selection_background_dialog(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_color_chooser_dialog_new("Choose Background Color", GTK_WINDOW(window));
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        
        char color_str[32];
        snprintf(color_str, sizeof(color_str), "rgba(%d,%d,%d,%.2f)",
                (int)(color.red * 255),
                (int)(color.green * 255),
                (int)(color.blue * 255),
                color.alpha);
        
        apply_background_to_selection(color_str);
    }
    
    gtk_widget_destroy(dialog);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Font dialog callback
void on_font_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_OK) {
        GtkFontChooser *chooser = GTK_FONT_CHOOSER(dialog);
        PangoFontDescription *font_desc = gtk_font_chooser_get_font_desc(chooser);
        
        if (font_desc) {
            // Extract font family
            const char *family = pango_font_description_get_family(font_desc);
            if (family) {
                g_free(current_font_family);
                current_font_family = g_strdup(family);
            }
            
            // Extract font size
            int size = pango_font_description_get_size(font_desc);
            if (size > 0) {
                current_font_size = size / PANGO_SCALE;
                if (current_font_size < 6) current_font_size = 6;
                if (current_font_size > 72) current_font_size = 72;
            }
            
            // Extract font weight and style
            PangoWeight weight = pango_font_description_get_weight(font_desc);
            font_bold = (weight >= PANGO_WEIGHT_BOLD);
            
            PangoStyle style = pango_font_description_get_style(font_desc);
            font_italic = (style == PANGO_STYLE_ITALIC || style == PANGO_STYLE_OBLIQUE);
            
            apply_current_font();
            pango_font_description_free(font_desc);
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

// Show font selection dialog
void show_font_dialog(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_font_chooser_dialog_new("Select Font", GTK_WINDOW(window));
    
    // Set current font
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s%s%d",
             current_font_family ? current_font_family : "Monospace",
             font_bold ? "Bold " : "",
             font_italic ? "Italic " : "",
             current_font_size);
    
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), font_string);
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_font_dialog_response), NULL);
    gtk_widget_show(dialog);
}

// Quick font family change functions
void change_font_family(const char *family) {
    if (current_font_family) g_free(current_font_family);
    current_font_family = g_strdup(family);
    apply_current_font();
}

void on_font_monospace(GtkWidget *widget, gpointer data) {
    change_font_family("Monospace");
}

void on_font_serif(GtkWidget *widget, gpointer data) {
    change_font_family("Serif");
}

void on_font_sans_serif(GtkWidget *widget, gpointer data) {
    change_font_family("Sans");
}

void on_font_courier(GtkWidget *widget, gpointer data) {
    change_font_family("Courier New");
}

void on_font_times(GtkWidget *widget, gpointer data) {
    change_font_family("Times New Roman");
}

void on_font_arial(GtkWidget *widget, gpointer data) {
    change_font_family("Arial");
}

// Font style toggles
void on_toggle_bold(GtkWidget *widget, gpointer data) {
    font_bold = !font_bold;
    apply_current_font();
}

void on_toggle_italic(GtkWidget *widget, gpointer data) {
    font_italic = !font_italic;
    apply_current_font();
}

// Font size presets
void set_font_size(int size) {
    if (size >= 6 && size <= 72) {
        current_font_size = size;
        apply_current_font();
    }
}
// Improved version of apply_current_font() function
void apply_current_font() {
    if (!text_view) {
        g_warning("text_view is NULL in apply_current_font");
        return;
    }

    if (!font_css_provider) {
        font_css_provider = gtk_css_provider_new();
    }

    char css[512];
    char weight[16] = "normal";
    char style[16] = "normal";
    
    if (font_bold) strcpy(weight, "bold");
    if (font_italic) strcpy(style, "italic");

    // Create CSS rule
    snprintf(css, sizeof(css),
             "textview { font-family: \"%s\"; font-size: %dpt; font-weight: %s; font-style: %s; }",
             current_font_family ? current_font_family : "Monospace",
             current_font_size,
             weight,
             style);

    // Load CSS
    GError *error = NULL;
    if (!gtk_css_provider_load_from_data(font_css_provider, css, -1, &error)) {
        g_warning("Failed to load CSS: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    }

    // Apply to text view
    GtkStyleContext *context = gtk_widget_get_style_context(text_view);
    
    // Remove previous provider if it exists
    gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER(font_css_provider));
    
    // Add new provider
    gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(font_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Also add to default screen for broader compatibility
    GdkScreen *screen = gtk_widget_get_screen(text_view);
    if (screen) {
        gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(font_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    // Force a redraw
    gtk_widget_queue_draw(text_view);
    
    g_print("Applied font: %s %dpt %s %s\n", 
            current_font_family ? current_font_family : "Monospace",
            current_font_size, weight, style);
}
void on_font_size_8(GtkWidget *widget, gpointer data) { set_font_size(8); }
void on_font_size_10(GtkWidget *widget, gpointer data) { set_font_size(10); }
void on_font_size_12(GtkWidget *widget, gpointer data) { set_font_size(12); }
void on_font_size_14(GtkWidget *widget, gpointer data) { set_font_size(14); }
void on_font_size_16(GtkWidget *widget, gpointer data) { set_font_size(16); }
void on_font_size_18(GtkWidget *widget, gpointer data) { set_font_size(18); }
void on_font_size_24(GtkWidget *widget, gpointer data) { set_font_size(24); }
void on_font_size_36(GtkWidget *widget, gpointer data) { set_font_size(36); }

// Legacy zoom functions (now use font system)
void apply_textview_font_size(int font_size) {
    current_font_size = font_size;
    apply_current_font();
}

void initialize_textview_font_size() {
    initialize_font_system();
}

// Handler for Zoom In
void on_zoom_in(GtkWidget *widget, gpointer data) {
    if (current_font_size < 48) { // Max font size
        current_font_size += 2;
        apply_current_font();
    }
}

// Handler for Zoom Out
void on_zoom_out(GtkWidget *widget, gpointer data) {
    if (current_font_size > 6) { // Min font size
        current_font_size -= 2;
        apply_current_font();
    }
}

// For search results navigation
static SearchResults current_results = {0};
static int current_match = -1;

// --- Utility Functions ---

void update_piece_table_from_buffer(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (doc_piecetable != NULL)
        piecetable_free(doc_piecetable);

    doc_piecetable = piecetable_create(text);
    g_free(text);
}

// --- Undo/Redo Integration ---

void on_begin_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    prev_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void on_end_user_action(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *new_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (prev_text)
        undo_redo_push(undo_stack, prev_text, new_text);

    g_free(prev_text);
    prev_text = NULL;
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

// This is done to select text for individual font size change
void apply_font_size_to_selection(GtkTextBuffer *buffer, int font_size) {
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end))
        return;

    // Create a unique tag for this font size if not already created
    char tag_name[32];
    snprintf(tag_name, sizeof(tag_name), "font_%d", font_size);

    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(tag_table, tag_name);

    if (!tag) {
        tag = gtk_text_buffer_create_tag(buffer, tag_name, "font", NULL, NULL);

        char font_desc[64];
        snprintf(font_desc, sizeof(font_desc), "Monospace %d", font_size);
        g_object_set(tag, "font", font_desc, NULL);
    }

    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
}

void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    update_piece_table_from_buffer();
}

// --- File Operations ---

void on_new(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkTextBuffer *buffer;
    gchar *filename = NULL;
    GError *error = NULL;

    dialog = gtk_file_chooser_dialog_new("Create New File",
                                         GTK_WINDOW(data),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Create", GTK_RESPONSE_ACCEPT,
                                         NULL);

    chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(chooser);

        // Try creating an empty file
        if (!g_file_set_contents(filename, "", 0, &error)) {
            g_printerr("Error creating file: %s\n", error->message);
            g_clear_error(&error);
        } else {
            // Clear the text buffer
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_set_text(buffer, "", -1);

            // Free and reset the piece table
            if (doc_piecetable != NULL)
                piecetable_free(doc_piecetable);
            doc_piecetable = piecetable_create("");

            // Free and update filename
            if (current_filename)
                g_free(current_filename);
            current_filename = g_strdup(filename);

            update_window_title(GTK_WINDOW(data), current_filename);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
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

            if (doc_piecetable != NULL)
                piecetable_free(doc_piecetable);

            doc_piecetable = piecetable_create(contents);

            // Store filename
            if (current_filename) g_free(current_filename);
            current_filename = g_strdup(filename);

            update_window_title(GTK_WINDOW(window), current_filename); // Update title

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
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    gchar *text;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (current_filename) {
        // Save to the current file
        if (!g_file_set_contents(current_filename, text, -1, NULL)) {
            g_print("Error saving file!\n");
        } else {
            update_window_title(GTK_WINDOW(window), current_filename);
        }
    } else {
        // No file yet: prompt user to choose a file
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
            GTK_WINDOW(window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL);

        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
        gtk_file_chooser_set_current_name(chooser, "Untitled.txt");

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(chooser);
            if (!g_file_set_contents(filename, text, -1, NULL)) {
                g_print("Error saving file!\n");
            } else {
                // Store filename and update title
                if (current_filename) g_free(current_filename);
                current_filename = g_strdup(filename);
                update_window_title(GTK_WINDOW(window), current_filename);
            }
            g_free(filename);
        }
        gtk_widget_destroy(dialog);
    }
    g_free(text);
}

void on_quit(GtkWidget *widget, gpointer data) {
    if (doc_piecetable != NULL)
        piecetable_free(doc_piecetable);
    if (undo_stack != NULL)
        undo_redo_stack_free(undo_stack);
    if (current_font_family)
        g_free(current_font_family);
    gtk_main_quit();
}

// --- Search Integration ---

void show_search_bar(GtkWidget *widget, gpointer data) {
    gtk_search_bar_set_search_mode(GTK_SEARCH_BAR(search_bar), TRUE);
    gtk_widget_grab_focus(GTK_WIDGET(search_entry));
}

void on_search_text_changed(GtkEntry *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    search_results_free(&current_results);

    if (strlen(text) > 0) {
        current_results = kmp_search(text, doc_piecetable);
        current_match = current_results.count > 0 ? 0 : -1;
    } else {
        current_match = -1;
    }

    if (current_match != -1) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        int match_offset = current_results.indices[current_match];
        int match_length = strlen(gtk_entry_get_text(entry));

        GtkTextIter match_start, match_end;
        gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
        match_end = match_start;
        gtk_text_iter_forward_chars(&match_end, match_length);

        gtk_text_buffer_select_range(buffer, &match_start, &match_end);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
    }
}

void on_next_match(GtkWidget *widget, gpointer data) {
    if (current_results.count == 0) return;
    current_match = (current_match + 1) % current_results.count;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    int match_offset = current_results.indices[current_match];
    int match_length = strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)));

    GtkTextIter match_start, match_end;
    gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
    match_end = match_start;
    gtk_text_iter_forward_chars(&match_end, match_length);

    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
}

void on_previous_match(GtkWidget *widget, gpointer data) {
    if (current_results.count == 0) return;
    current_match = (current_match - 1 + current_results.count) % current_results.count;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    int match_offset = current_results.indices[current_match];
    int match_length = strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)));

    GtkTextIter match_start, match_end;
    gtk_text_buffer_get_iter_at_offset(buffer, &match_start, match_offset);
    match_end = match_start;
    gtk_text_iter_forward_chars(&match_end, match_length);

    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
}

void on_search_bar_close(GtkSearchBar *search_bar, gpointer user_data) {
    gtk_search_bar_set_search_mode(search_bar, FALSE);
}

void on_replace_clicked(GtkWidget *widget, gpointer user_data) {
    GtkEntry *replace_entry_local = GTK_ENTRY(user_data);
    const gchar *replace_text = gtk_entry_get_text(replace_entry_local);

    if (current_results.count == 0 || current_match < 0) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    int match_offset = current_results.indices[current_match];
    int match_length = strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)));

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_offset(buffer, &start, match_offset);
    end = start;
    gtk_text_iter_forward_chars(&end, match_length);

    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_insert(buffer, &start, replace_text, -1);

    // Refresh search results after replacement
    on_search_text_changed(GTK_ENTRY(search_entry), NULL);
}

void on_replace_all_clicked(GtkWidget *widget, gpointer user_data) {
    GtkEntry *replace_entry_local = GTK_ENTRY(user_data);
    const gchar *replace_text = gtk_entry_get_text(replace_entry_local);
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));

    if (!search_text || strlen(search_text) == 0) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, match_start, match_end;
    gtk_text_buffer_get_start_iter(buffer, &start);

    while (gtk_text_iter_forward_search(&start, search_text, GTK_TEXT_SEARCH_VISIBLE_ONLY, &match_start, &match_end, NULL)) {
        gtk_text_buffer_delete(buffer, &match_start, &match_end);
        gtk_text_buffer_insert(buffer, &match_start, replace_text, -1);
        // Move start to after the replaced text
        start = match_start;
        gtk_text_iter_forward_chars(&start, strlen(replace_text));
    }
    // Refresh search results
    on_search_text_changed(GTK_ENTRY(search_entry), NULL);
}

// --- Key Press Handler (Undo/Redo, Bracket Auto-close, Search) ---

gboolean on_text_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter cursor_iter;
    gchar open_char = 0;
    gchar close_char = 0;

    // Auto-close brackets
    switch (event->keyval) {
        case GDK_KEY_parenleft: open_char = '('; close_char = ')'; break;
        case GDK_KEY_bracketleft: open_char = '['; close_char = ']'; break;
        case GDK_KEY_braceleft: open_char = '{'; close_char = '}'; break;
        case GDK_KEY_less: open_char = '<'; close_char = '>'; break;
        default: break;
    }

    if (open_char != 0 && close_char != 0) {
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, gtk_text_buffer_get_insert(buffer));
        gchar text[3] = {open_char, close_char, '\0'};
        gtk_text_buffer_insert(buffer, &cursor_iter, text, 2);

        // Move cursor between brackets
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, gtk_text_buffer_get_insert(buffer));
        gtk_text_iter_backward_char(&cursor_iter);
        gtk_text_buffer_place_cursor(buffer, &cursor_iter);

        return TRUE; // Skip default handling
    }

    // Ctrl+F: Show search bar
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_f)) {
        show_search_bar(NULL, NULL);
        return TRUE;
    }

    // Ctrl+Z: Undo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_z)) {
        on_undo(NULL, NULL);
        return TRUE;
    }

    // Ctrl+Y: Redo
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_y)) {
        on_redo(NULL, NULL);
        return TRUE;
    }

    // Ctrl+S: Save
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_s)) {
        on_save(NULL, gtk_widget_get_toplevel(widget));
        return TRUE;
    }

    // Ctrl+B: Toggle Bold
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_b)) {
        on_toggle_bold(NULL, NULL);
        return TRUE;
    }

    // Ctrl+I: Toggle Italic
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_i)) {
        on_toggle_italic(NULL, NULL);
        return TRUE;
    }

    // Font size shortcuts (Ctrl+1 through Ctrl+9)
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_1)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 10);
        return TRUE;
    }

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_2)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 15);
        return TRUE;
    }

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_3)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 20);
        return TRUE;
    }

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_4)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 30);
        return TRUE;
    }

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_5)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 40);
        return TRUE;
    }

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_6)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        apply_font_size_to_selection(buffer, 50);
        return TRUE;
    }

// Ctrl+7 → font size 60
if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_7)) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    apply_font_size_to_selection(buffer, 30);
    return TRUE;
}
// Ctrl+8 → font size 70
if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_8)) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    apply_font_size_to_selection(buffer, 30);
    return TRUE;
}
// Ctrl+9 → font size 80
if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_9)) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    apply_font_size_to_selection(buffer, 80);
    return TRUE;
}



    return FALSE; // Let normal keys pass through
}
