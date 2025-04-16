#include <stdint.h>
#include "arch/asm.h"
#include "drivers/pit.h"

// Channel
#define PIT_CHANNEL_0 (0b00 << 6)
#define PIT_CHANNEL_1 (0b01 << 6)
#define PIT_CHANNEL_2 (0b10 << 6)

// Access mode
#define PIT_AM_LOHI (0b11 << 4)

// Operating mode
#define PIT_OM_RATE_GENERATOR (0b11 << 1)

// Set PIT channel 0 to the desired frequency
void set_pit_channel_0(uint32_t frequency){
  uint32_t div = 1193180 / frequency;
  outb(0x43, PIT_CHANNEL_0 | PIT_AM_LOHI | PIT_OM_RATE_GENERATOR);
  outb(0x40, (uint8_t) (div) );
  outb(0x40, (uint8_t) (div >> 8));
}

// Set PIT channel 2 to the desired frequency
void set_pit_channel_2(uint32_t frequency){
  uint32_t div = 1193180 / frequency;
  outb(0x43, PIT_CHANNEL_2 | PIT_AM_LOHI | PIT_OM_RATE_GENERATOR);
  outb(0x42, (uint8_t) (div) );
  outb(0x42, (uint8_t) (div >> 8));
}
