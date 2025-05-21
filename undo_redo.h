#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include "list.h"

typedef struct undo_redo_action {
    char *prev_text;
    char *next_text;
} UndoRedoAction;

typedef struct undo_redo_stack {
    List actions;
    int current_index;
} UndoRedoStack;

UndoRedoStack* undo_redo_stack_create(void);
void undo_redo_stack_free(UndoRedoStack *stack);
void undo_redo_push(UndoRedoStack *stack, const char *prev, const char *next);
int undo_redo_can_undo(UndoRedoStack *stack);
int undo_redo_can_redo(UndoRedoStack *stack);
const char* undo_redo_undo(UndoRedoStack *stack);
const char* undo_redo_redo(UndoRedoStack *stack);

#endif // UNDO_REDO_H
