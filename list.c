#include <stdlib.h>
#include "list.h"

List list_create(void) {
    List l = malloc(sizeof(struct list));
    l->first = NULL;
    l->last = NULL;
    l->length = 0;
    return l;
}

void list_free(List l) {
    ListItem curr = l->first;
    ListItem next;
    while (curr) {
        next = curr->next;
        free(curr->value);
        free(curr);
        curr = next;
    }
    free(l);
}

ListItem list_get_first(List l) {
    return l->first;
}

ListItem list_get_last(List l) {
    return l->last;
}

int list_length(List l) {
    return l->length;
}

void list_append(List l, void *value) {
    ListItem new = (ListItem)malloc(sizeof(struct list_item));
    new->value = value;
    new->next = NULL;
    if (l->first == NULL) {
        l->first = new;
        l->last = new;
    } else {
        l->last->next = new;
        l->last = new;
    }
    l->length++;
}

ListItem list_get_item(List l, int i) {
    ListItem curr = l->first;
    int curr_idx = 0;
    while (curr && curr_idx < i) {
        curr = curr->next;
        curr_idx++;
    }
    return curr;
}

ListItem list_insert(List l, int idx, void *value) {
    ListItem new = (ListItem)malloc(sizeof(struct list_item));
    new->value = value;
    if (idx == 0) {
        new->next = l->first;
        l->first = new;
        if (l->length == 0) {
            l->last = new;
        }
    } else {
        ListItem before = list_get_item(l, idx - 1);
        if (before == NULL) {
            free(new);
            return NULL;
        } else {
            new->next = before->next;
            before->next = new;
            if (new->next == NULL)
                l->last = new;
        }
    }
    l->length++;
    return new;
}
