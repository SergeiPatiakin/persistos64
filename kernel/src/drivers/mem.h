#ifndef MEM_H
#define MEM_H
#include "fs/vfs.h"

extern struct file_operations dev_mem_fops;

void dev_mem_init();

#endif
