#include <string.h>
#include <stdlib.h>
#include "search.h"
#include "piecetable.h"

static void compute_lps(const char *pattern, int *lps) {
    int len = 0;
    lps[0] = 0;
    for (int i = 1; pattern[i];) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else {
            if (len) len = lps[len - 1];
            else lps[i++] = 0;
        }
    }
}

SearchResults kmp_search(const char *pattern, Piecetable pt) {
    SearchResults results = {0};
    char *text = piecetable_value(pt);
    if (!text) return results; // Defensive: handle NULL
    int M = strlen(pattern);
    int N = strlen(text);

    if (M == 0 || N == 0 || M > N) {
        free(text);
        return results;
    }

    int *lps = malloc(M * sizeof(int));
    compute_lps(pattern, lps);

    int *matches = malloc(N * sizeof(int)); // FIXED: allocate for all possible matches
    int count = 0;

    int i = 0, j = 0;
    while (i < N) {
        if (pattern[j] == text[i]) {
            j++;
            i++;
        }
        if (j == M) {
            matches[count++] = i - j;
            j = lps[j - 1];
        } else if (i < N && pattern[j] != text[i]) {
            if (j) j = lps[j - 1];
            else i++;
        }
    }

    free(lps);
    free(text);

    results.indices = matches;
    results.count = count;
    return results;
}

void search_results_free(SearchResults *results) {
    free(results->indices);
    results->indices = NULL;
    results->count = 0;
}
