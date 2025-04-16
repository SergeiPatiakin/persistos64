#ifndef ZERO_H
#define ZERO_H
#include "fs/vfs.h"

extern struct file_operations zero_device_fops;

void zero_init();

#endif
