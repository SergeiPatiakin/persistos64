#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdint.h>

#define O_CREAT 0x1
#define O_TRUNCATE 0x2

#define SEEK_SET 0x0

struct fb_info {
    uint16_t fb_width;
    uint16_t fb_height;
    uint16_t fb_pitch;
};

struct dent_header {
    uint16_t len;
} __attribute__((packed));

#define TS_RUNNING 0x91
#define TS_WAITING 0x92
#define TS_ZOMBIE 0x93

struct tent_header {
    uint32_t pid;
    uint8_t state;
    uint16_t len;
} __attribute__((packed));

#endif
