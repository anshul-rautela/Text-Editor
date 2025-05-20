#include <gtk/gtk.h>
#include <string.h>
#include "matching.h"

// Tag names for parenthesis highlighting
#define HIGHLIGHT_TAG "paren_highlight"
#define MISMATCH_TAG "paren_mismatch"

// Store the applied tags so we can remove them later
static GtkTextTag *highlight_tag = NULL;
static GtkTextTag *mismatch_tag = NULL;
static GtkTextMark *last_highlight_mark_start = NULL;
static GtkTextMark *last_highlight_mark_end = NULL;

/**
 * Checks if the character is an opening bracket
 */
static gboolean is_opening_bracket(gchar ch) {
    return (ch == '(' || ch == '[' || ch == '{' || ch == '<');
}

/**
 * Checks if the character is a closing bracket
 */
static gboolean is_closing_bracket(gchar ch) {
    return (ch == ')' || ch == ']' || ch == '}' || ch == '>');
}


/**
 * Returns the matching closing bracket for an opening bracket
 */
static gchar get_matching_bracket(gchar ch) {
    switch (ch) {
        case '(': return ')';
        case '[': return ']';
        case '{': return '}';
        case '<': return '>';
        case ')': return '(';
        case ']': return '[';
        case '}': return '{';
        case '>': return '<';
        default: return '\0';
    }
}


void highlight_brackets(GtkTextBuffer *buffer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    // First remove old bracket tags
    gtk_text_buffer_remove_tag_by_name(buffer, "bracket_level_1", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "bracket_level_2", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "bracket_level_3", &start, &end);

    GtkTextIter iter = start;
    int level = 0;

    while (!gtk_text_iter_equal(&iter, &end)) {
        gunichar ch = gtk_text_iter_get_char(&iter);
        const char *tag_name = NULL;

        if (ch == '(' || ch == '[' || ch == '{') {
            level++;
        }

        if (ch == ')' || ch == ']' || ch == '}') {
            // Assign current level before decrementing
            if (level <= 3) {
                tag_name = g_strdup_printf("bracket_level_%d", level);
                gtk_text_buffer_apply_tag_by_name(buffer, tag_name, &iter, gtk_text_iter_copy(&iter) + 1);
                g_free((char *)tag_name);
            }
            level--;
        } else if (ch == '(' || ch == '[' || ch == '{') {
            if (level <= 3) {
                tag_name = g_strdup_printf("bracket_level_%d", level);
                gtk_text_buffer_apply_tag_by_name(buffer, tag_name, &iter, gtk_text_iter_copy(&iter) + 1);
                g_free((char *)tag_name);
            }
        }

        gtk_text_iter_forward_char(&iter);
    }
}



/**
 * Remove existing highlight tags from the buffer
 */
static void remove_highlight_tags(GtkTextBuffer *buffer) {
    if (last_highlight_mark_start && last_highlight_mark_end) {
        GtkTextIter start, end;
        
        gtk_text_buffer_get_iter_at_mark(buffer, &start, last_highlight_mark_start);
        gtk_text_buffer_get_iter_at_mark(buffer, &end, last_highlight_mark_end);
        
        // Remove all highlight tags
        gtk_text_buffer_remove_tag_by_name(buffer, HIGHLIGHT_TAG, &start, &end);
        gtk_text_buffer_remove_tag_by_name(buffer, MISMATCH_TAG, &start, &end);
        
        // Delete the marks
        gtk_text_buffer_delete_mark(buffer, last_highlight_mark_start);
        gtk_text_buffer_delete_mark(buffer, last_highlight_mark_end);
        
        last_highlight_mark_start = NULL;
        last_highlight_mark_end = NULL;
    }
}
static void ensure_tags_created(GtkTextBuffer *buffer) {
    if (!highlight_tag) {
        highlight_tag = gtk_text_buffer_create_tag(buffer, HIGHLIGHT_TAG,
                                                  "foreground", "#9CDCFE",  // Changed from background
                                                  NULL);
    }

    if (!mismatch_tag) {
        mismatch_tag = gtk_text_buffer_create_tag(buffer, MISMATCH_TAG,
                                                 "foreground", "#F44747",  // Changed from background
                                                 NULL);
    }
}

void highlight_matching_bracket(GtkTextView *text_view, GtkTextIter *iter) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter cursor = *iter;
    GtkTextIter match_pos;
    gboolean found_match = FALSE;
    gboolean is_mismatch = FALSE;
    gchar current_char, match_char;
    
    // First, remove any existing highlight
    remove_highlight_tags(buffer);
    
    // // Ensure our highlight tags exist
    ensure_tags_created(buffer);
    
    // Get the character at or before the cursor
    if (!gtk_text_iter_is_start(&cursor)) {
        gtk_text_iter_backward_char(&cursor);
    }
    
    current_char = gtk_text_iter_get_char(&cursor);
    
    // If it's not a bracket, nothing to do
    if (!is_opening_bracket(current_char) && !is_closing_bracket(current_char)) {
        return;
    }
    
    match_char = get_matching_bracket(current_char);
    match_pos = cursor;
    
    // Find the matching bracket
    if (is_opening_bracket(current_char)) {
        // Search forward for closing bracket
        int bracket_level = 1;
        
        while (gtk_text_iter_forward_char(&match_pos)) {
            gchar ch = gtk_text_iter_get_char(&match_pos);
            
            if (ch == current_char) {
                bracket_level++;
            } else if (ch == match_char) {
                bracket_level--;
                if (bracket_level == 0) {
                    found_match = TRUE;
                    break;
                }
            }
        }
    } else if (is_closing_bracket(current_char)) {
        // Search backward for opening bracket
        int bracket_level = 1;
        
        while (gtk_text_iter_backward_char(&match_pos)) {
            gchar ch = gtk_text_iter_get_char(&match_pos);
            
            if (ch == current_char) {
                bracket_level++;
            } else if (ch == match_char) {
                bracket_level--;
                if (bracket_level == 0) {
                    found_match = TRUE;
                    break;
                }
            }
        }
    }
    
    // Apply the tags to highlight the brackets
    GtkTextIter start_cursor = cursor;
    GtkTextIter end_cursor = cursor;
    gtk_text_iter_forward_char(&end_cursor);
    
    const gchar *tag_name = found_match ? HIGHLIGHT_TAG : MISMATCH_TAG;
    
    // Highlight the current bracket
    gtk_text_buffer_apply_tag_by_name(buffer, tag_name, &start_cursor, &end_cursor);
    
    // Save this position with marks for later removal
    last_highlight_mark_start = gtk_text_buffer_create_mark(buffer, NULL, &start_cursor, TRUE);
    last_highlight_mark_end = gtk_text_buffer_create_mark(buffer, NULL, &end_cursor, FALSE);
    
    if (found_match) {
        // Highlight the matching bracket
        GtkTextIter match_end = match_pos;
        gtk_text_iter_forward_char(&match_end);
        
        gtk_text_buffer_apply_tag_by_name(buffer, tag_name, &match_pos, &match_end);
    }
}

/**
 * Handle cursor movements to update bracket highlighting
 */
void on_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer data) {
    // Only process cursor position changes
    if (mark == gtk_text_buffer_get_insert(buffer)) {
        GtkTextView *text_view = GTK_TEXT_VIEW(data);
        highlight_matching_bracket(text_view, location);
    }
}

/**
 * Initialize parenthesis matching for a text view
 */
void init_bracket_matching(GtkTextView *text_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    // // Create the tags for highlighting
    ensure_tags_created(buffer);
    
    // Connect to the mark-set signal to track cursor movement
    g_signal_connect(buffer, "mark-set", G_CALLBACK(on_mark_set), text_view);
}