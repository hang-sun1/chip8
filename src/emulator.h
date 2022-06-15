#pragma once

#include <stdint.h>

typedef struct Emulator {
    uint8_t memory[4096];
    uint16_t pc;
     
} Emulator;