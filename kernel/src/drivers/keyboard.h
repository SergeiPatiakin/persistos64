#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>

#define KBD_ASCII 0xD1
#define KBD_NONASCII 0xD2

#define KBD_ESC 0xE1
#define KBD_BACKSPACE 0xE2
#define KBD_ARROW_UP 0XE3
#define KBD_ARROW_DOWN 0xE4
#define KBD_ARROW_RIGHT 0xE5
#define KBD_ARROW_LEFT 0xE6

struct keyboard_event {
    uint8_t symbol_type; // KBD_ASCII, KBD_NONASCII
    uint8_t symbol;
    uint8_t scancode;
};

void keyboard_rb_fill();

#endif
