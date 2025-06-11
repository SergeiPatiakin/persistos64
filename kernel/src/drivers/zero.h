#ifndef ZERO_H
#define ZERO_H
#include "fs/vfs.h"

extern struct device_operations zero_device_ops;

void dev_zero_init();

#endif
