#ifndef ZERO_H
#define ZERO_H
#include "fs/vfs.h"

extern struct file_operations dev_zero_fops;

void dev_zero_init();

#endif
