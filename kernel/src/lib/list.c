#include "lib/list.h"

void init_list(struct list_head *head) {
  head->prev = head;
  head->next = head;
}

void list_add(struct list_head *new_entry, struct list_head *head) {
  struct list_head *following = head->next;
  new_entry->prev = head;
  head->next = new_entry;
  following->prev = new_entry;
  new_entry->next = following;
}

void list_add_tail(struct list_head *new_entry, struct list_head *head) {
  struct list_head *preceding = head->prev;
  new_entry->prev = preceding;
  preceding->next = new_entry;
  new_entry->next = head;
  head->prev = new_entry;
}

void list_del(struct list_head *entry) {
  struct list_head *following = entry->next;
  struct list_head *preceding = entry->prev;
  following->prev = preceding;
  preceding->next = following;
}
