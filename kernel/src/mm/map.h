#ifndef MAP_H
#define MAP_H

void set_page_mapping(void *pml4_page, void* virt_address, void* phys_address, bool is_mmio);

#endif
