#pragma once

#include "SDL_render.h"
#include <SDL.h>
#include <stdint.h>

#define MEMORY_SIZE 4096
#define PROGRAM_START_OFFSET 512
#define MAX_PROGRAM_SIZE (MEMORY_SIZE - PROGRAM_START_OFFSET)
#define STACK_SIZE 1024

// width is 8 bytes or 64 bits
#define DISPLAY_WIDTH 64
// height is 4 bytes or 32 bits
#define DISPLAY_HEIGHT 32

typedef struct Emulator {
  uint8_t memory[MEMORY_SIZE];
  uint16_t stack[STACK_SIZE];
  // sp == Stack pointer
  uint16_t sp;
  // program counter
  uint16_t pc;
  // The registers, named/indexed V0 - VF
  uint8_t registers[16];
  uint16_t index_register;
    
  uint64_t graphics[DISPLAY_HEIGHT];
} Emulator;

void init_emulator(Emulator *emulator);
void load_program(Emulator *emulator, uint8_t *program, long program_size);
void run(Emulator *emulator);
void render_grid(SDL_Renderer *r, double width, double height);
void render_display(SDL_Renderer *r, double width, double height, Emulator* emulator);