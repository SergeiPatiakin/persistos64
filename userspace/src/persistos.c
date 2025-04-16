#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void *heap_bottom;
void *heap_top;
void *program_break;
void libc_init() {
    heap_top = heap_bottom = program_break = brk(NULL);
}

void *malloc(size_t size) {
    void *old_heap_top = heap_top;
    heap_top += size;
    if (heap_top > program_break) {
        program_break = brk(heap_top);
    }
    return old_heap_top;
}

void free(void *addr) {
    (void)addr; // unused
    // free is a no-op in bump allocator
}

struct FILE std_files[3] = {
    {._fd = 0},
    {._fd = 1},
    {._fd = 2},
};
struct FILE* stdin = std_files;
struct FILE* stdout = &std_files[1];
struct FILE* stderr = &std_files[2];

void puts(uint8_t* s) {
    write(1, s, strlen(s));
}

void fputs(uint8_t* s, struct FILE* file) {
    write(file->_fd, s, strlen(s));
}

bool is_error(ssize_t x) {
    return (x >= -4095) && (x <= -1);
}
