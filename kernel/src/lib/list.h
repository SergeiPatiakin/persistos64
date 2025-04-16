#ifndef LIST_H
#define LIST_H

#include <stddef.h>

// Doubly linked circular list
struct list_head {
  struct list_head *prev;
  struct list_head *next;
};

void init_list(struct list_head *head);
void list_add(struct list_head *new_entry, struct list_head *head);
void list_add_tail(struct list_head *new_entry, struct list_head *head);
void list_del(struct list_head *entry);

#define container_of(ptr, type, member) ((type*)((void*)ptr - offsetof(type,member)))
#define list_for_each(x, y) for (struct list_head *x = y.next; x != &y; x = x->next)

#endif
