#ifndef LIST_H
#define LIST_H

typedef struct list_item {
    void *value;
    struct list_item *next;
} *ListItem;

typedef struct list {
    ListItem first;
    ListItem last;
    int length;
} *List;

List list_create(void);
void list_free(List l);
ListItem list_get_first(List l);
ListItem list_get_last(List l);
int list_length(List l);
void list_append(List l, void *value);
ListItem list_get_item(List l, int i);
ListItem list_insert(List l, int idx, void *value);

#endif // LIST_H
