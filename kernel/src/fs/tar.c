#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "arch/asm.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "fs/tar.h"
#include "fs/vfs.h"

#define FILEPATH_MAX_LENGTH 400

void extract_tar_files(void *tar_file) {
    uint8_t filepath_buffer[FILEPATH_MAX_LENGTH + 1];
    uint8_t filename_buffer[FILEPATH_MAX_LENGTH + 1];
    while (true) {
        struct tar_posix_header *header = tar_file;
        if (memcmp(header->size, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 12) == 0) {
            return;
        }
        uint64_t file_size = 0;
        parse_oct(header->size, &file_size);

        size_t filepath_strlen = strlen(header->name);
        if (filepath_strlen > FILEPATH_MAX_LENGTH) {
            panic(u8p("File path too long"));
        }
        strcpy(filepath_buffer, header->name);
        
        // Remove trailing slash
        if (filepath_buffer[filepath_strlen - 1] == '/') {
            filepath_buffer[filepath_strlen - 1] = 0;
            filepath_strlen -= 1;
        }

        struct vfs_lookup_result lookup_result;
        vfs_resolve(filepath_buffer, &lookup_result);
        if (lookup_result.status != VFS_RESOLVE_SUCCESS_DOESNT_EXIST) {
            panic(u8p("Unexpected resolve status while extracting tar"));
        }
        memset(&filename_buffer, 0, FILEPATH_MAX_LENGTH + 1);
        memcpy(&filename_buffer, lookup_result.name_start, lookup_result.name_length);
        if (header->typeflag == '5') {
            // directory
            vfs_mkdir(lookup_result.parent_directory, filename_buffer);
        } else {
            // file
            struct inode *inode = vfs_create(lookup_result.parent_directory, filename_buffer);
            struct file filp = {
                .fd = 0,
                .inode = inode,
                .files_le = {.next = NULL, .prev = NULL},
                .offset = 0,
            };
            vfs_write(&filp, tar_file + 512, file_size);
        }
        uint64_t increment_blocks = 1 + (file_size >> 9) + ((file_size & 0x1FF) != 0 ? 1 : 0);
        tar_file += (increment_blocks << 9);
    }
}
