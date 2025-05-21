#ifndef PIECETABLE_H
#define PIECETABLE_H

#include "list.h"

#define ORIGINAL 0
#define ADD 1

typedef struct piece {
    int which;    // 0 = "original", 1 = "add"
    int start;
    int length;
} *Piece;

typedef struct piecetable {
    char *original;
    List add;      // List of added strings
    List pieces;   // List of Piece
    int length;    // Character count of the current value
} *Piecetable;

Piecetable piecetable_create(char *original);
void piecetable_free(Piecetable pt);
int piecetable_add_length(Piecetable pt);
void piecetable_insert(Piecetable pt, char *value, int at);
char *piecetable_value(Piecetable pt);

#endif // PIECETABLE_H
