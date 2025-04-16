#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void sprintf_uint64(uint64_t value, uint8_t *buffer);
void sprintf_uint32(uint32_t value, uint8_t *buffer);
void sprintf_uint16(uint16_t data, uint8_t *buffer);
void sprintf_uint8(uint8_t value, uint8_t *buffer);
void sprintf_dec(uint32_t data, uint8_t *result, uint8_t pad_char, uint8_t max_digits);

// String functions
int8_t strcmp(uint8_t *buf1, uint8_t *buf2);
int8_t strncmp(uint8_t *buf1, uint8_t *buf2, size_t n);
int8_t strklcmp(uint8_t *cstr, uint8_t *klstr, size_t n);
size_t strlen(uint8_t* str);
uint8_t* strcpy(uint8_t* dest, const uint8_t* src);

// Returns number of character parsed
uint8_t parse_oct(uint8_t *str, uint64_t *result);

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

typedef signed long long ssize_t;
#define u8p(x) ((uint8_t*)(x))
