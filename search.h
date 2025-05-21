#ifndef SEARCH_H
#define SEARCH_H

#include "piecetable.h"

typedef struct {
    int *indices;
    int count;
} SearchResults;

SearchResults kmp_search(const char *pattern, Piecetable pt);
void search_results_free(SearchResults *results);

#endif // SEARCH_H
