#ifndef SYSFS_H
#define SYSFS_H
#include <stdint.h>
#include "fs/vfs.h"
#include "lib/list.h"
#include "mm/slab.h"

struct sysfs_superblock {};
struct sysfs_inode {};

extern struct filesystem_ops sysfs_superblock_ops;
extern struct inode sysfs_dev_inode;

void sysfs_init();

#endif
