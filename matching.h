#ifndef MATCHING_H
#define MATCHING_H

#include <gtk/gtk.h>

/**
 * Highlights matching parentheses/brackets at the current cursor position
 */
void highlight_matching_bracket(GtkTextView *text_view, GtkTextIter *iter);

/**
 * Handle cursor movements to update bracket highlighting
 */
void on_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer data);

/**
 * Initialize parenthesis matching for a text view
 */
void init_bracket_matching(GtkTextView *text_view);


void highlight_brackets(GtkTextBuffer *buffer);
void init_bracket_tags(GtkTextBuffer *buffer);

#endif /* MATCHING_H */