#ifndef GDT_H
#define GDT_H

void add_usermode_gdt_entries();
void add_tss_gdt_entry(void *tss);

#endif
