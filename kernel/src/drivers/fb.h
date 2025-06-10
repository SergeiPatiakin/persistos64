#ifndef FB_H
#define FB_H
#include "lib/cstd.h"

void fb_init();
ssize_t fb_ioctl(void *dev, uint64_t cmd, uint64_t arg);

#endif
