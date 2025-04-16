#include <stdint.h>
#include <stdbool.h>
#include "keyboard.h"
#include "arch/asm.h"
#include "drivers/tty.h"

struct keyboard_event keymap_no_modifiers[128] = {
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ESC}, // 0x1 - escape
    
    {.symbol_type = KBD_ASCII, .symbol = '1'},
    {.symbol_type = KBD_ASCII, .symbol = '2'},
    {.symbol_type = KBD_ASCII, .symbol = '3'},
    {.symbol_type = KBD_ASCII, .symbol = '4'},
    {.symbol_type = KBD_ASCII, .symbol = '5'},
    {.symbol_type = KBD_ASCII, .symbol = '6'},
    {.symbol_type = KBD_ASCII, .symbol = '7'},
    {.symbol_type = KBD_ASCII, .symbol = '8'},
    {.symbol_type = KBD_ASCII, .symbol = '9'},
    {.symbol_type = KBD_ASCII, .symbol = '0'},
    {.symbol_type = KBD_ASCII, .symbol = '-'},
    {.symbol_type = KBD_ASCII, .symbol = '='},
    {.symbol_type = KBD_NONASCII, .symbol = KBD_BACKSPACE}, // 0x0E - backspace

    {.symbol_type = KBD_ASCII, .symbol = '\t'},
    {.symbol_type = KBD_ASCII, .symbol = 'q'},
    {.symbol_type = KBD_ASCII, .symbol = 'w'},
    {.symbol_type = KBD_ASCII, .symbol = 'e'},
    {.symbol_type = KBD_ASCII, .symbol = 'r'},
    {.symbol_type = KBD_ASCII, .symbol = 't'},
    {.symbol_type = KBD_ASCII, .symbol = 'y'},
    {.symbol_type = KBD_ASCII, .symbol = 'u'},
    {.symbol_type = KBD_ASCII, .symbol = 'i'},
    {.symbol_type = KBD_ASCII, .symbol = 'o'},
    {.symbol_type = KBD_ASCII, .symbol = 'p'},
    {.symbol_type = KBD_ASCII, .symbol = '['},
    {.symbol_type = KBD_ASCII, .symbol = ']'},
    {.symbol_type = KBD_ASCII, .symbol = '\n'}, // 0x1C - enter
    
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x1D - ctrl
    {.symbol_type = KBD_ASCII, .symbol = 'a'},
    {.symbol_type = KBD_ASCII, .symbol = 's'},
    {.symbol_type = KBD_ASCII, .symbol = 'd'},
    {.symbol_type = KBD_ASCII, .symbol = 'f'},
    {.symbol_type = KBD_ASCII, .symbol = 'g'},
    {.symbol_type = KBD_ASCII, .symbol = 'h'},
    {.symbol_type = KBD_ASCII, .symbol = 'j'},
    {.symbol_type = KBD_ASCII, .symbol = 'k'},
    {.symbol_type = KBD_ASCII, .symbol = 'l'},
    {.symbol_type = KBD_ASCII, .symbol = ';'},
    {.symbol_type = KBD_ASCII, .symbol = '\''},
    {.symbol_type = KBD_ASCII, .symbol = '`'}, // 0x29
    
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x2A - shift
    {.symbol_type = KBD_ASCII, .symbol = '#'}, // 0x2B
    {.symbol_type = KBD_ASCII, .symbol = 'z'},
    {.symbol_type = KBD_ASCII, .symbol = 'x'},
    {.symbol_type = KBD_ASCII, .symbol = 'c'},
    {.symbol_type = KBD_ASCII, .symbol = 'v'},
    {.symbol_type = KBD_ASCII, .symbol = 'b'},
    {.symbol_type = KBD_ASCII, .symbol = 'n'},
    {.symbol_type = KBD_ASCII, .symbol = 'm'},
    {.symbol_type = KBD_ASCII, .symbol = ','},
    {.symbol_type = KBD_ASCII, .symbol = '.'},
    {.symbol_type = KBD_ASCII, .symbol = '/'},
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x36 - right shift

    {.symbol_type = KBD_ASCII, .symbol = '*'}, // 0x37 - numpad multiply
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x38 - alt
    {.symbol_type = KBD_ASCII, .symbol = ' '},	// 0x39 - spacebar
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x3A - caps lock
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x44 - F10 key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x45 - num lock
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x46 - scroll lock
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x47 - home key
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_UP}, // 0x48 - up arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x49 - page up
    {.symbol_type = KBD_ASCII, .symbol = '-'}, // 0x4A - numpad subtract
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_LEFT}, // 0x4B - left arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x4C - numpad 5 / clear
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_RIGHT}, // 0x4D - right arrow
    {.symbol_type = KBD_ASCII, .symbol = '+'}, // 0x4E - numpad add
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x4F - end key
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_DOWN}, // 0x50 - down arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x51 - numpad 3 / page down
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x52 - insert key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x53 - delete key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x54 - printscreen?
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_ASCII, .symbol = '\\'}, // 0x56
    {.symbol_type = KBD_NONASCII, .symbol = 0},	// 0x57 - F11 key
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    // All other keys are undefined
};

struct keyboard_event keymap_shift[128] = {
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ESC}, // 0x1 - escape

    {.symbol_type = KBD_ASCII, .symbol = '!'},
    {.symbol_type = KBD_ASCII, .symbol = '"'},
    {.symbol_type = KBD_ASCII, .symbol = '#'},
    {.symbol_type = KBD_ASCII, .symbol = '$'}, // TODO: pound sign
    {.symbol_type = KBD_ASCII, .symbol = '%'},
    {.symbol_type = KBD_ASCII, .symbol = '^'},
    {.symbol_type = KBD_ASCII, .symbol = '&'},
    {.symbol_type = KBD_ASCII, .symbol = '*'},
    {.symbol_type = KBD_ASCII, .symbol = '('},
    {.symbol_type = KBD_ASCII, .symbol = ')'},
    {.symbol_type = KBD_ASCII, .symbol = '_'},
    {.symbol_type = KBD_ASCII, .symbol = '+'},
    {.symbol_type = KBD_NONASCII, .symbol = KBD_BACKSPACE}, // 0x0E - backspace

    {.symbol_type = KBD_ASCII, .symbol = '\t'},
    {.symbol_type = KBD_ASCII, .symbol = 'Q'},
    {.symbol_type = KBD_ASCII, .symbol = 'W'},
    {.symbol_type = KBD_ASCII, .symbol = 'E'},
    {.symbol_type = KBD_ASCII, .symbol = 'R'},
    {.symbol_type = KBD_ASCII, .symbol = 'T'},
    {.symbol_type = KBD_ASCII, .symbol = 'Y'},
    {.symbol_type = KBD_ASCII, .symbol = 'U'},
    {.symbol_type = KBD_ASCII, .symbol = 'I'},
    {.symbol_type = KBD_ASCII, .symbol = 'O'},
    {.symbol_type = KBD_ASCII, .symbol = 'P'},
    {.symbol_type = KBD_ASCII, .symbol = '{'},
    {.symbol_type = KBD_ASCII, .symbol = '}'},
    {.symbol_type = KBD_ASCII, .symbol = '\n'}, // 0x1C - enter

    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x1D - ctrl
    {.symbol_type = KBD_ASCII, .symbol = 'A'},
    {.symbol_type = KBD_ASCII, .symbol = 'S'},
    {.symbol_type = KBD_ASCII, .symbol = 'D'},
    {.symbol_type = KBD_ASCII, .symbol = 'F'},
    {.symbol_type = KBD_ASCII, .symbol = 'G'},
    {.symbol_type = KBD_ASCII, .symbol = 'H'},
    {.symbol_type = KBD_ASCII, .symbol = 'J'},
    {.symbol_type = KBD_ASCII, .symbol = 'K'},
    {.symbol_type = KBD_ASCII, .symbol = 'L'},
    {.symbol_type = KBD_ASCII, .symbol = ':'},
    {.symbol_type = KBD_ASCII, .symbol = '@'},
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x29 - TODO: negation sign??

    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x2A - shift
    {.symbol_type = KBD_ASCII, .symbol = '~'}, // 0x2B
    {.symbol_type = KBD_ASCII, .symbol = 'Z'},
    {.symbol_type = KBD_ASCII, .symbol = 'X'},
    {.symbol_type = KBD_ASCII, .symbol = 'C'},
    {.symbol_type = KBD_ASCII, .symbol = 'V'},
    {.symbol_type = KBD_ASCII, .symbol = 'B'},
    {.symbol_type = KBD_ASCII, .symbol = 'N'},
    {.symbol_type = KBD_ASCII, .symbol = 'M'},
    {.symbol_type = KBD_ASCII, .symbol = '<'},
    {.symbol_type = KBD_ASCII, .symbol = '>'},
    {.symbol_type = KBD_ASCII, .symbol = '?'},
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x36 - right shift

    {.symbol_type = KBD_ASCII, .symbol = '*'}, // 0x37 - numpad multiply
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x38 - alt
    {.symbol_type = KBD_ASCII, .symbol = ' '},	// 0x39 - spacebar
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x3A - caps lock
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x44 - F10 key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x45 - num lock
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x46 - scroll lock
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x47 - home key
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_UP}, // 0x48 - up arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x49 - page up
    {.symbol_type = KBD_ASCII, .symbol = '-'}, // 0x4A - numpad subtract
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_LEFT}, // 0x4B - left arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x4C - numpad 5 / clear
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_RIGHT}, // 0x4D - right arrow
    {.symbol_type = KBD_ASCII, .symbol = '+'}, // 0x4E - numpad add
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x4F - end key
    {.symbol_type = KBD_NONASCII, .symbol = KBD_ARROW_DOWN}, // 0x50 - down arrow
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x51 - numpad 3 / page down
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x52 - insert key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x53 - delete key
    {.symbol_type = KBD_NONASCII, .symbol = 0}, // 0x54 - printscreen?
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    {.symbol_type = KBD_ASCII, .symbol = '|'}, // 0x56
    {.symbol_type = KBD_NONASCII, .symbol = 0},	// 0x57 - F11 key
    {.symbol_type = KBD_NONASCII, .symbol = 0},
    // All other keys are undefined
};

#define MOD_CTRL  (1 << 0)
#define MOD_SHIFT (1 << 1)
#define MOD_ALT   (1 << 2)

/* The modifier keys currently pressed */
static uint8_t depressed_mod_keys = 0;

// #define KEYBOARD_BUFFER_LENGTH 256
// size_t keyboard_rb_head_idx = 0;
// size_t keyboard_rb_tail_idx = 0;
// uint8_t keyboard_rb[KEYBOARD_BUFFER_LENGTH];

// This function can be called in interrupt context
void keyboard_rb_fill() {
    bool has_more_keys;
    do {
        uint8_t scancode = inb(0x60);
        has_more_keys = inb(0x64) & 0x1;
        // if (
        //     (KEYBOARD_BUFFER_LENGTH + keyboard_rb_head_idx - keyboard_rb_tail_idx)
        //     % KEYBOARD_BUFFER_LENGTH
        //     == 1
        // ) {
        //     // If head is 1 ahead of tail, the ring buffer is full so we have to drop events
        //     continue;
        // }
        switch(scancode) {
        case 0x2A: // Shift down
            depressed_mod_keys |= MOD_SHIFT;
            break;
        case 0xAA: // Shift up
            depressed_mod_keys &= ~MOD_SHIFT;
            break;
        case 0x1D: // Ctrl down
            depressed_mod_keys |= MOD_CTRL;
            break;
        case 0x9D: // Ctrl up
            depressed_mod_keys &= ~MOD_CTRL;
            break;
        case 0x38: // Alt down
            depressed_mod_keys |= MOD_ALT;
            break;
        case 0xB8: // Alt up
            depressed_mod_keys &= ~MOD_ALT;
            break;
        default:
            if (scancode > 0x80) {
                // Ignore other key up events
                break;
            }
            if (scancode == 0x02 && (depressed_mod_keys & MOD_CTRL)) {
                // Ctrl + 1 -> switch to tty1
                set_active_vt(&tty1);
                break;
            }
            if (scancode == 0x03 && (depressed_mod_keys & MOD_CTRL)) {
                // Ctrl + 2 -> switch to tty2
                set_active_vt(&tty2);
                break;
            }
            if (scancode == 0x04 && (depressed_mod_keys & MOD_CTRL)) {
                // Ctrl + 3 -> switch to tty3
                set_active_vt(&tty3);
                break;
            }
            struct keyboard_event event = (depressed_mod_keys & MOD_SHIFT) ? keymap_shift[scancode] : keymap_no_modifiers[scancode];
            event.scancode = scancode;
            if (event.symbol == 0) {
                break;
            }
            // keyboard_rb[keyboard_rb_tail_idx] = character;
            // keyboard_rb_tail_idx = (keyboard_rb_tail_idx + 1) % KEYBOARD_BUFFER_LENGTH;
            vt_update_input(active_vt_device, event);
            // TODO: wait_queue_wake_and_destroy(&active_vt_device->keyboard_waitqueue_head);
        }
    } while (has_more_keys);
}
