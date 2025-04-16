#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cstd.h"

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// String functions

// Public. Buffer must have length at least 17
void sprintf_uint64(uint64_t value, uint8_t *buffer) {
    for (int i = 0; i < 16; i++) {
        uint8_t hex_digit = (value & 0x0f);
        uint8_t ascii_character;
        if (hex_digit < 10) {
            ascii_character = '0' + hex_digit;
        } else {
            ascii_character = 'A' + hex_digit - 10;
        }
        buffer[15 - i] = ascii_character;
        value = value >> 4;
    }
    buffer[16] = 0;
}

// Public. Buffer must have length at least 9
void sprintf_uint32(uint32_t value, uint8_t *buffer) {
    for (int i = 0; i < 8; i++) {
        uint8_t hex_digit = (value & 0x0f);
        uint8_t ascii_character;
        if (hex_digit < 10) {
            ascii_character = '0' + hex_digit;
        } else {
            ascii_character = 'A' + hex_digit - 10;
        }
        buffer[7 - i] = ascii_character;
        value = value >> 4;
    }
    buffer[8] = 0;
}

// Public. Buffer must have length at least 5
void sprintf_uint16(uint16_t data, uint8_t *buffer) {
    for (int i = 0; i < 4; i++) {
        uint8_t hex_digit = (data & 0x0f);
        uint8_t ascii_character;
        if (hex_digit < 10) {
            ascii_character = '0' + hex_digit;
        } else {
            ascii_character = 'A' + hex_digit - 10;
        }
        buffer[3 - i] = ascii_character;
        data = data >> 4;
    }
    buffer[4] = 0;
}

// Public. Buffer must have length at least 3
void sprintf_uint8(uint8_t value, uint8_t *buffer) {
    for (int i = 0; i < 2; i++) {
        uint8_t hex_digit = (value & 0x0f);
        uint8_t ascii_character;
        if (hex_digit < 10) {
            ascii_character = '0' + hex_digit;
        } else {
            ascii_character = 'A' + hex_digit - 10;
        }
        buffer[1 - i] = ascii_character;
        value = value >> 4;
    }
    buffer[2] = 0;
}

// result must have length at least 11
// If pad_char is 0, no padding will be applied
// max_digits must be between 0 and 10
// if max_digits is 0, no maximum will be applied
void sprintf_dec(uint32_t data, uint8_t *result, uint8_t pad_char, uint8_t max_digits) {
    memset(result, 0, 11);
    uint8_t buffer[10];
    uint8_t first_nonzero_digit_index = 9;
    for (int i = 0; i < 10; i++) {
        uint8_t hex_digit = data % 10;
        buffer[9 - i] = '0' + hex_digit;
        if (hex_digit != 0) {
            first_nonzero_digit_index = 9 - i;
        }
        data = data / 10;
    }
    if (pad_char != 0) {
        // With padding
        for (int i = 0; i < first_nonzero_digit_index; i++) {
            buffer[i] = pad_char;
        }
        memcpy(result, buffer + 10 - max_digits, max_digits);
    } else {
        // With no padding
        uint8_t num_digits;
        if (max_digits == 0) {
            num_digits = 10 - first_nonzero_digit_index;
        } else if (max_digits >= 10 - first_nonzero_digit_index) {
            num_digits = 10 - first_nonzero_digit_index;
        } else {
            num_digits = max_digits;
        }
        memcpy(result, buffer + 10 - num_digits, num_digits);
    }
}

int8_t strcmp(uint8_t *buf1, uint8_t *buf2) {
    for (;*buf1 == *buf2 && *buf1 != 0 && *buf2 != 0; buf1++, buf2++);
    return *buf1 - *buf2;
}

int8_t strncmp(uint8_t *buf1, uint8_t *buf2, size_t n) {
    size_t i = 0;
    for (; *buf1 == *buf2 && *buf1 != 0 && *buf2 != 0 && i<n; i++, buf1++, buf2++);
    return (i == n) ? 0 : (*buf1 - *buf2);
}

// Compare C string to known length string
int8_t strklcmp(uint8_t *cstr, uint8_t *klstr, size_t n) {
    size_t m = strlen(cstr);
    if (m > n) {
        int8_t x = strncmp(cstr, klstr, n);
        if (x) {
        return x;
        }
        return 1; // cstr is longer and starts with klstr
    } else if (m < n) {
        int8_t x = strncmp(cstr, klstr, m);
        if (x) {
        return x;
        }
        return 1; // klstr is longer and starts with cstr
    } else return strncmp(cstr, klstr, n);
}

size_t strlen(uint8_t *str) {
	size_t len = 0;
	while (str[len]) len++;
	return len;
}

uint8_t* strcpy(uint8_t* dest, const uint8_t* src) {
    size_t i = 0;
    do {
        *((uint8_t*)dest + i) = *((uint8_t*)src + i);
    } while (*((uint8_t*)src + i++));
    return dest;
}

// Returns number of character parsed
uint8_t parse_oct(uint8_t *str, uint64_t *result) {
    uint64_t tmp = 0;
    uint8_t *c;
    c = str;
    // Push bits from high bits toward low bits
    while (true) {
        if (*c >= '0' && *c <= '7') {
            tmp <<= 3;
            tmp += (*c - '0');
        } else {
            break;
        }
        c++;
    }
    *result = tmp;
    return c - str;
}

// Parse up to n characters at str. Returns number of character parsed
uint8_t parse_n_dec(uint8_t *str, uint32_t n, uint64_t *result) {
    uint64_t tmp = 0;
    uint8_t *c;
    c = str;
    // Push bits from high bits toward low bits
    while (c - str < n) {
        if (*c >= '0' && *c <= '9') {
        tmp *= 10;
        tmp += (*c - '0');
        } else {
        break;
        }
        c++;
    }
    *result = tmp;
    return c - str;
}
