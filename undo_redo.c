#include <stdlib.h>
#include <string.h>
#include "undo_redo.h"

UndoRedoStack* undo_redo_stack_create(void) {
    UndoRedoStack *stack = malloc(sizeof(UndoRedoStack));
    stack->actions = list_create();
    stack->current_index = -1;
    return stack;
}

void undo_redo_stack_free(UndoRedoStack *stack) {
    ListItem item = list_get_first(stack->actions);
    while (item) {
        UndoRedoAction *action = (UndoRedoAction*)item->value;
        free(action->prev_text);
        free(action->next_text);
        free(action);
        item = item->next;
    }
    list_free(stack->actions);
    free(stack);
}

void undo_redo_push(UndoRedoStack *stack, const char *prev, const char *next) {
    while (list_length(stack->actions) > stack->current_index + 1) {
        ListItem last = list_get_last(stack->actions);
        UndoRedoAction *action = (UndoRedoAction*)last->value;
        free(action->prev_text);
        free(action->next_text);
        free(action);
        ListItem item = stack->actions->first;
        if (item == last) {
            stack->actions->first = NULL;
            stack->actions->last = NULL;
        } else {
            while (item->next != last) item = item->next;
            item->next = NULL;
            stack->actions->last = item;
        }
        stack->actions->length--;
        free(last);
    }
    UndoRedoAction *action = malloc(sizeof(UndoRedoAction));
    action->prev_text = strdup(prev);
    action->next_text = strdup(next);
    list_append(stack->actions, action);
    stack->current_index++;
}

int undo_redo_can_undo(UndoRedoStack *stack) {
    return stack->current_index >= 0;
}

int undo_redo_can_redo(UndoRedoStack *stack) {
    return stack->current_index < list_length(stack->actions) - 1;
}

const char* undo_redo_undo(UndoRedoStack *stack) {
    if (!undo_redo_can_undo(stack)) return NULL;
    ListItem item = list_get_item(stack->actions, stack->current_index);
    stack->current_index--;
    UndoRedoAction *action = (UndoRedoAction*)item->value;
    return action->prev_text;
}

const char* undo_redo_redo(UndoRedoStack *stack) {
    if (!undo_redo_can_redo(stack)) return NULL;
    stack->current_index++;
    ListItem item = list_get_item(stack->actions, stack->current_index);
    UndoRedoAction *action = (UndoRedoAction*)item->value;
    return action->next_text;
}
