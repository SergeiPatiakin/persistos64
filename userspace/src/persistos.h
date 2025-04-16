#ifndef PERSISTOS_H
#define PERSISTOS_H
#include <stdint.h>
#include <stdbool.h>

// persistos.c

void *malloc(size_t size);
void free(void *addr);

struct FILE {
    uint64_t _fd;
};
extern struct FILE* stdin;
extern struct FILE* stdout;
extern struct FILE* stderr;

void puts(uint8_t* s);
void fputs(uint8_t* s, struct FILE* file);
bool is_error(ssize_t x);

// persistos.s

/* 1  */ ssize_t write(uint64_t fd, uint8_t *buf, size_t size);
/* 2  */ ssize_t read(uint64_t fd, uint8_t *buf, size_t size);
/* 3  */ void exit(uint8_t exit_code);
/* 4  */ ssize_t getpid();
/* 5  */ void sched_yield();
/* 6  */ ssize_t fork();
/* 7  */ ssize_t exec(uint8_t *path, uint8_t **argv);
/* 8  */ void *brk(void *new_brk);
/* 9  */ ssize_t waitpid(uint64_t pid, uint64_t *status_pointer);
/* 10 */ ssize_t open(uint8_t *path, uint64_t options);
/* 11 */ ssize_t close(uint64_t fd);
/* 12 */ ssize_t getdents(uint64_t fd, uint8_t *buf, size_t size);
/* 13 */ ssize_t mkdir(uint8_t *path);
/* 14 */ ssize_t lseek(uint64_t fd, uint64_t offset, int whence);
/* 15 */ ssize_t ftruncate(uint64_t fd, uint64_t size);
/* 16 */ ssize_t dup2(uint64_t old_fd, uint64_t new_fd);
/* 17 */ ssize_t gettasks(uint8_t *buf, size_t size);
/* 18 */ ssize_t kill(uint64_t pid, uint64_t sig);
/* 19 */ ssize_t sleep(uint64_t millis);
/* 20 */ ssize_t mount(uint8_t *dev_name, uint8_t *dir_name, uint8_t *type);

#define O_CREAT 0x1
#define O_TRUNCATE 0x2

#endif
