#ifndef LIMINE_REQUESTS_H
#define LIMINE_REQUESTS_H
#include "lib/limine.h"

extern volatile uint64_t limine_base_revision[3];

__attribute__((section(".requests")))
extern volatile struct limine_framebuffer_request framebuffer_request;

__attribute__((section(".requests")))
extern volatile struct limine_hhdm_request hhdm_request;

__attribute__((section(".requests")))
extern volatile struct limine_memmap_request memmap_request;

__attribute__((section(".requests")))
extern volatile struct limine_module_request module_request;

#endif
