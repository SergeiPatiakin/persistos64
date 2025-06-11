#ifndef MEM_H
#define MEM_H
#include "fs/vfs.h"

extern struct device_operations mem_device_ops;

void dev_mem_init();

#endif
