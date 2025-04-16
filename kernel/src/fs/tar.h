#ifndef TAR_H
#define TAR_H
#include <stdint.h>
#include <stddef.h>
#include "lib/cstd.h"

/* tar Header Block, from POSIX 1003.1-1990.  */

struct tar_posix_header
{                                    /* byte offset */
    uint8_t name[100];               /*   0 */
    uint8_t mode[8];                 /* 100 */
    uint8_t uid[8];                  /* 108 */
    uint8_t gid[8];                  /* 116 */
    uint8_t size[12];                /* 124 - size in octal ASCII */
    uint8_t mtime[12];               /* 136 */
    uint8_t chksum[8];               /* 148 */
    uint8_t typeflag;                /* 156 */
    uint8_t linkname[100];           /* 157 */
    uint8_t magic[6];                /* 257 */
    uint8_t version[2];              /* 263 */
    uint8_t uname[32];               /* 265 */
    uint8_t gname[32];               /* 297 */
    uint8_t devmajor[8];             /* 329 */
    uint8_t devminor[8];             /* 337 */
    uint8_t prefix[155];             /* 345 */
                                     /* 500 */
};

ct_assert(sizeof(struct tar_posix_header) == 500);

void extract_tar_files(void *tar_file);

#endif
