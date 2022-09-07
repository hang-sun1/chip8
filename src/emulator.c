#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_render.h"
#include "emulator.h"

// Offset into main memeory where fonts are stored
static const uint16_t FONT_OFFSET = 0x50;

// Size of a font character in bytes
static const uint16_t FONT_SIZE = 5;

// Byte representations of characters supported by the font
static const uint8_t NUM_ZERO[] = {0xf0, 0x90, 0x90, 0x90, 0xf0};
static const uint8_t NUM_ONE[] = {0x20, 0x60, 0x20, 0x20, 0x70};
static const uint8_t NUM_TWO[] = {0xF0, 0x10, 0xF0, 0x80, 0xF0};
static const uint8_t NUM_THREE[] = {0xF0, 0x10, 0xF0, 0x10, 0xF0};
static const uint8_t NUM_FOUR[] = {0x90, 0x90, 0xF0, 0x10, 0x10};
static const uint8_t NUM_FIVE[] = {0xF0, 0x80, 0xF0, 0x10, 0xF0};
static const uint8_t NUM_SIX[] = {0xF0, 0x80, 0xF0, 0x90, 0xF0};
static const uint8_t NUM_SEVEN[] = {0xF0, 0x10, 0x20, 0x40, 0x40};
static const uint8_t NUM_EIGHT[] = {0xF0, 0x90, 0xF0, 0x90, 0xF0};
static const uint8_t NUM_NINE[] = {0xF0, 0x90, 0xF0, 0x10, 0xF0};
static const uint8_t LETTER_A[] = {0xF0, 0x90, 0xF0, 0x90, 0x90};
static const uint8_t LETTER_B[] = {0xF0, 0x90, 0xF0, 0x90, 0x90};
static const uint8_t LETTER_C[] = {0xF0, 0x80, 0x80, 0x80, 0xF0};
static const uint8_t LETTER_D[] = {0xE0, 0x90, 0x90, 0x90, 0xE0};
static const uint8_t LETTER_E[] = {0xF0, 0x80, 0xF0, 0x80, 0xF0};
static const uint8_t LETTER_F[] = {0xF0, 0x80, 0xF0, 0x80, 0x80};

void chip8_init_emulator(Chip8Emulator *emulator) {
  memset(emulator->memory, 0, MEMORY_SIZE);
  memset(emulator->stack, 0, STACK_SIZE);
  emulator->sp = 0;
  emulator->pc = 0;

  // Load the fonts, starting at the offset defined by FONT_OFFSET
  uint16_t offset = FONT_OFFSET;
  memcpy(emulator->memory + offset, NUM_ZERO, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_ONE, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_TWO, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_THREE, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_FOUR, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_FIVE, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_SIX, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_SEVEN, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_EIGHT, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, NUM_NINE, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_A, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_B, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_C, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_D, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_E, FONT_SIZE);
  offset += FONT_SIZE;
  memcpy(emulator->memory + offset, LETTER_F, FONT_SIZE);
  assert(offset == 0x9f - FONT_SIZE + 1);
}

void chip8_load_program(Chip8Emulator *emulator, uint8_t *program,
                        long program_size) {
  assert(program_size <= MAX_PROGRAM_SIZE);
  assert(program_size >= 0);
  memcpy(emulator->memory + 0x200, program, program_size);
  emulator->pc = 0x200;
}

// Reads the next instruction from memory and
// increments the program counter in preparation
// for the next instruction
static uint16_t fetch(Chip8Emulator *emulator) {
  assert(emulator->pc + 1 < MEMORY_SIZE);
  uint16_t first = (uint16_t)emulator->memory[emulator->pc];
  uint16_t second = (uint16_t)emulator->memory[emulator->pc + 1];
  emulator->pc += 2;

  return (first << 8) | second;
}

static inline uint16_t X(uint16_t ins) { return (ins & 0x0f00) >> 8; }

static inline uint16_t Y(uint16_t ins) { return (ins & 0x00f0) >> 4; }

static inline uint16_t N(uint16_t ins) { return ins & 0x000f; }

static inline uint16_t NN(uint16_t ins) { return ins & 0x00ff; }

static inline uint16_t NNN(uint16_t ins) { return ins & 0x0fff; }

static inline void jump(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t dest = NNN(ins);
  emulator->pc = dest;
}

static inline void set_register(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t reg = X(ins);
  uint16_t val = NN(ins);
  emulator->registers[reg] = val;
}

static inline void add_to_register(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t reg = X(ins);
  uint16_t val = NN(ins);
  emulator->registers[reg] += val;
  emulator->registers[reg] = emulator->registers[reg] % 256;
}

static inline void set_index_register(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t val = NNN(ins);
  emulator->index_register = val;
}

static inline void draw(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t x = emulator->registers[X(ins)] & 63;
  uint16_t y = emulator->registers[Y(ins)] & 31;
  uint16_t sprite_height = N(ins);

  for (int i = 0; i < sprite_height; i++) {
    if (y >= CHIP8_DISPLAY_HEIGHT) {
      break;
    }
    uint64_t data = (uint64_t) emulator->memory[emulator->index_register+i];

    emulator->graphics[y] ^= data << 56 >> x;

    // moving to the next row and exiting if the sprite
    // is cut off from the rest of the screen
    y += 1;
  }
  // TODO: set the flags register to 0
}

static inline void clear_screen(Chip8Emulator *emulator) {
  // just zero the entire graphics area
  memset(emulator->graphics, 0, 8 * 32);
}

static inline void call(Chip8Emulator *emulator, uint16_t ins) {
  emulator->stack[emulator->sp] = emulator->pc;
  emulator->sp += 1;
  uint16_t dest = NNN(ins);
  emulator->pc = dest;
}

static inline void pop(Chip8Emulator *emulator) {
  emulator->sp -= 1;
  emulator->pc = emulator->stack[emulator->sp];
}

static inline void skip3(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t x = emulator->registers[X(ins)];
  uint16_t nn = NN(ins);
  if (x == nn) {
    emulator->pc += 2;
  }
}

static inline void skip4(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t x = emulator->registers[X(ins)];
  uint16_t nn = NN(ins);
  if (x != nn) {
    emulator->pc += 2;
  }
}

static inline void skip5(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t x = emulator->registers[X(ins)];
  uint16_t y = emulator->registers[Y(ins)];

  if (x == y) {
    emulator->pc += 2;
  }
}

static inline void skip6(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t x = emulator->registers[X(ins)];
  uint16_t y = emulator->registers[Y(ins)];

  if (x != y) {
    emulator->pc += 2;
  }
}

static inline void arithmetic(Chip8Emulator *emulator, uint16_t ins) {
  uint16_t opcode = ins & 0xf;
  uint16_t *x = &emulator->registers[X(ins)];
  uint16_t y = emulator->registers[Y(ins)];

  switch (opcode) {

    // set
    case 0:
    *x = y;
    break;

    // or
    case 1:
    *x |= y;
    break;

    // and 
    case 2:
    *x &= y;
    break;

    // xor
    case 3:
    *x ^= y;
    break;

    // add
    case 4:
    *x += y;
    if (*x > 255) {
      emulator->registers[0xf] = 1;
      x -= 256;
    }
    break;

    // subtract x - y
    case 5:
    *x = *x - y;
    break;

    // subtract y - x
    case 6:
    *x = y - *x;
  }
}

// Takes in the 16 bit instruction, decoding it into its component
// parts and running it
static void decode_and_execute(Chip8Emulator *emulator, uint16_t instruction) {
  // The instruction's "name" is the first 4 bits of the instruction
  uint16_t instruction_type = (instruction & 0xf000) >> 12;
   
  switch (instruction_type) {
  case 0x0:
    clear_screen(emulator);
    // for now just clear the screen
    // TODO: clear the screen
    break;

  case 0x1:
    jump(emulator, instruction);
    break;

  case 0x6:
    set_register(emulator, instruction);
    break;

  case 0x7:
    add_to_register(emulator, instruction);
    break;

  case 0xa:
    set_index_register(emulator, instruction);
    break;

  case 0xd:
    draw(emulator, instruction);
    break;

  default:
    puts("unrecognized instruction found");
    break;
  }
}

void chip8_run(Chip8Emulator *emulator) {
  uint16_t ins = fetch(emulator);
  decode_and_execute(emulator, ins);
}

void chip8_render_grid(SDL_Renderer *r, double width, double height) {
  SDL_SetRenderDrawColor(r, 0xff, 0xff, 0, 0x0f);
  for (int x = 1; x < CHIP8_DISPLAY_WIDTH; x++) {
    SDL_RenderDrawLine(r, width / 64.0 * x, 0, width / 64 * x, height);
  }

  for (int y = 1; y < CHIP8_DISPLAY_HEIGHT; y++) {
    SDL_RenderDrawLine(r, 0, height / 32.0 * y, width, height / 32 * y);
  }
}

// TODO: lower number of drawcalls by calling SDL_RenderFillRects for all 32 * 64 pixels at once
void chip8_render_display(SDL_Renderer *r, double width, double height,
                          Chip8Emulator *emulator) {
  for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
    for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
      SDL_Rect pixel = {width / 64.0 * x, height / 32.0 * y,
                        width / 64.0 * (x + 1), height / 32.0 * (y + 1)};
      int color = emulator->graphics[y] & (0x8000000000000000 >> x) ? 0xff : 0x00;
      SDL_SetRenderDrawColor(r, color, color, color, 0xff);
      SDL_RenderFillRect(r, &pixel);
    }
  }
}
