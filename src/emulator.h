#pragma once

#include "SDL_render.h"
#include <SDL.h>
#include <stdint.h>

#define MEMORY_SIZE 4096
#define PROGRAM_START_OFFSET 512
#define MAX_PROGRAM_SIZE (MEMORY_SIZE - PROGRAM_START_OFFSET)
#define STACK_SIZE 1024

// width is 8 bytes or 64 bits
#define CHIP8_DISPLAY_WIDTH 64
// height is 4 bytes or 32 bits
#define CHIP8_DISPLAY_HEIGHT 32

typedef struct Emulator {
  uint8_t memory[MEMORY_SIZE];
  uint16_t stack[STACK_SIZE];
  // sp == Stack pointer
  uint16_t sp;
  // program counter
  uint16_t pc;
  // The registers, named/indexed V0 - VF
  uint16_t registers[16];
  uint16_t index_register;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint64_t graphics[CHIP8_DISPLAY_HEIGHT];
} Chip8Emulator;

void chip8_init_emulator(Chip8Emulator *emulator);
void chip8_load_program(Chip8Emulator *emulator, uint8_t *program,
                        long program_size);
void chip8_run(Chip8Emulator *emulator);
void chip8_render_grid(SDL_Renderer *r, double width, double height);
void chip8_render_display(SDL_Renderer *r, double width, double height,
                          Chip8Emulator *emulator);
