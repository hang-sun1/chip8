#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_events.h"
#include "SDL_keycode.h"
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
  memset(emulator, 0, sizeof(Chip8Emulator));
  
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
  emulator->registers[0xf] = 0;
  uint16_t x = emulator->registers[X(ins)] & 63;
  uint16_t y = emulator->registers[Y(ins)] & 31;
  uint16_t sprite_height = N(ins);

  for (int i = 0; i < sprite_height; i++) {
    if (y >= CHIP8_DISPLAY_HEIGHT) {
      break;
    }
    uint64_t data = (uint64_t)emulator->memory[emulator->index_register + i];

    uint64_t new_bytes = (data << 56) >> x;

    if (new_bytes & emulator->graphics[y]) {
      emulator->registers[0xf] = 1;
    }

    emulator->graphics[y] ^= new_bytes;

    // moving to the next row and exiting if the sprite
    // is cut off from the rest of the screen
    y += 1;
  }

  // TODO: set the flags register to 0
}

static inline void clear_screen(Chip8Emulator *emulator) {
  // just zero the entire graphics area
  memset(emulator->graphics, 0, sizeof(emulator->graphics));
}

static inline void call(Chip8Emulator *emulator, uint16_t ins) {
  emulator->sp += 1;
  emulator->stack[emulator->sp] = emulator->pc;
  uint16_t dest = NNN(ins);
  emulator->pc = dest;
}

static inline void pop(Chip8Emulator *emulator) {
  emulator->pc = emulator->stack[emulator->sp];
  emulator->sp -= 1;
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
      *x &= 0xff;
      emulator->registers[0xf] = 1;
    } else {
      emulator->registers[0xf] = 0;
    }
    break;

  // subtract x - y
  case 5:
    
    if (*x >= y) {
      *x = *x - y;
      emulator->registers[0xf] = 1;
    } else {
      uint8_t xsmall = *x;
      uint8_t ysmall = y;
      uint8_t sum = xsmall - ysmall;

      // *x = 0;
      *x = sum;
      // *x = y - *x;
      // *x = 255 + *x - y;
      emulator->registers[0xf] = 0;
    }
    break;

  // shift right
  case 6: 
    *x = y;
    emulator->registers[0xf] = *x & 0x01;
    *x = *x >> 1;
    break;

  // subtract y - x
  case 7:
    if (y >= *x) {
      *x = y - *x;
      emulator->registers[0xf] = 1;
    } else {
      emulator->registers[0xf] = 0;
      uint8_t xsmall = *x;
      uint8_t ysmall = y;
      uint8_t sum = ysmall - xsmall;
      *x = sum;
    }
    break;

  // shift left
  case 0xe:
    *x = y;
    uint16_t shifted2 = (*x & 0x80) >> 7;
    *x = (*x << 1) & 0xff;
    emulator->registers[0xf] = shifted2;
    break;
  }
}

static void jump_with_offset(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t nnn = NNN(instruction);
  emulator->pc = emulator->registers[0] + nnn;
}

static void chip8_random(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t random = (uint16_t)rand() % 256;
  uint16_t nn = NN(instruction);
  uint16_t x = X(instruction);
  emulator->registers[x] = random & nn;
}

static void binary_decimal_convert(Chip8Emulator *emulator,
                                   uint16_t instruction) {
  uint8_t num = emulator->registers[X(instruction)];

  uint8_t biggest_digit = num / 100;
  num = num % 100;
  uint8_t middle_digit = num / 10;
  num = num % 10;
  uint8_t smallest_digit = num;

  emulator->memory[emulator->index_register] = biggest_digit;
  emulator->memory[emulator->index_register + 1] = middle_digit;
  emulator->memory[emulator->index_register + 2] = smallest_digit;
}

static void store_memory(Chip8Emulator *emulator, uint16_t instruction) {
  for (int i = 0; i <= X(instruction); i++) {
    emulator->memory[emulator->index_register + i] = emulator->registers[i];
  }
}

static void load_memory(Chip8Emulator *emulator, uint16_t instruction) {
  for (int i = 0; i <= X(instruction); i++) {
    emulator->registers[i] = emulator->memory[emulator->index_register + i];
  }
}

static void read_display_timer(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t x = X(instruction);
  emulator->registers[x] = emulator->delay_timer;
}

static void set_display_timer(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t x = X(instruction);
  emulator->delay_timer = emulator->registers[x];
}

static void set_sound_timer(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t x = X(instruction);
  emulator->sound_timer = emulator->registers[x];
}

static void set_index_to_font(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t character = emulator->registers[X(instruction)] & 0xf;
  emulator->index_register = FONT_OFFSET + character * FONT_SIZE;
}

static void add_to_index(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t x = emulator->registers[X(instruction)];
  emulator->index_register += x;
}

static void chip8_handle_f_instructions(Chip8Emulator *emulator,
                                        uint16_t instruction) {
  uint16_t lower_half = instruction & 0xff;

  switch (lower_half) {
  case 0x07:
    read_display_timer(emulator, instruction);
    break;

  case 0x15:
    set_display_timer(emulator, instruction);
    break;

  case 0x18:
    set_sound_timer(emulator, instruction);
    break;

  case 0x29:
    set_index_to_font(emulator, instruction);
    break;

  case 0x33:
    binary_decimal_convert(emulator, instruction);
    break;

  case 0x55:
    store_memory(emulator, instruction);
    break;

  case 0x65:
    load_memory(emulator, instruction);
    break;
  
  case 0x1e:
    add_to_index(emulator, instruction);
    break;

  default:
    printf("unrecognized instruction %x\n", instruction);
    break;
  }
}

static void chip8_handle_e_instructions(Chip8Emulator *emulator, uint16_t instruction) {
  uint16_t lower_half = instruction & 0xff;

  uint16_t x = emulator->registers[X(instruction)];
  switch (lower_half) {
  case 0x9e:
  if (emulator->inputs[x]) {
    emulator->pc += 2;
    puts("hello");
  }
  break;
  
  case 0xa1:
  if (!emulator->inputs[x]) {
    emulator->pc += 2;
  }
  break;

  
  default:
    printf("unrecognized instruction %x\n", instruction);
    break;
  }
}

// Takes in the 16 bit instruction, decoding it into its component
// parts and running it
static void decode_and_execute(Chip8Emulator *emulator, uint16_t instruction) {
  // The instruction's "name" is the first 4 bits of the instruction
  uint16_t instruction_type = (instruction & 0xf000) >> 12;

  switch (instruction_type) {
  case 0x0:
    if (instruction == 0x00e0) {
      clear_screen(emulator);
    } else if (instruction == 0x00ee) {
      pop(emulator);
    } else {
      printf("unrecognized instruction %x\n", instruction);
    }
    break;

  case 0x1:
    jump(emulator, instruction);
    break;

  case 0x2:
    call(emulator, instruction);
    break;

  case 0x3:
    skip3(emulator, instruction);
    break;

  case 0x4:
    skip4(emulator, instruction);
    break;

  case 0x5:
    skip5(emulator, instruction);
    break;

  case 0x6:
    set_register(emulator, instruction);
    break;

  case 0x7:
    add_to_register(emulator, instruction);
    break;

  case 0x8:
    arithmetic(emulator, instruction);
    break;

  case 0x9:
    skip6(emulator, instruction);
    break;

  case 0xa:
    set_index_register(emulator, instruction);
    break;

  case 0xb:
    jump_with_offset(emulator, instruction);
    break;

  case 0xc:
    chip8_random(emulator, instruction);
    break;

  case 0xd:
    draw(emulator, instruction);
    break;

  case 0xe:
    chip8_handle_e_instructions(emulator, instruction);
    break;

  case 0xf:
    chip8_handle_f_instructions(emulator, instruction);
    break;

  default:
    printf("unrecognized instruction %x\n", instruction);
    break;
  }
}

static void handle_timers(Chip8Emulator *emulator, uint32_t delta_t) {
  if (emulator->delay_timer) {
    emulator->delay_timer_acc += delta_t; 

    if (emulator->delay_timer_acc >= 16) {
      emulator->delay_timer_acc -= 16;
      emulator->delay_timer -= 1;
    }   
  }

  if (emulator->sound_timer) {
    emulator->sound_timer_acc += delta_t;

    if (emulator->sound_timer_acc >= 16) {
      emulator->sound_timer_acc -= 16;
      emulator->sound_timer -= 1;
    }   
  }
}

void chip8_run(Chip8Emulator *emulator, uint64_t delta_t, SDL_Event event) {
  uint16_t ins = fetch(emulator);
  decode_and_execute(emulator, ins);
  handle_timers(emulator, delta_t);
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

// TODO: lower number of drawcalls by calling SDL_RenderFillRects for all 32 *
// 64 pixels at once
void chip8_render_display(SDL_Renderer *r, double width, double height,
                          Chip8Emulator *emulator) {
  for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
    for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
      SDL_Rect pixel = {width / 64.0 * x, height / 32.0 * y,
                        width / 64.0 * (x + 1), height / 32.0 * (y + 1)};
      int color =
          emulator->graphics[y] & (0x8000000000000000 >> x) ? 0xff : 0x00;
      SDL_SetRenderDrawColor(r, color, color, color, 0xff);
      SDL_RenderFillRect(r, &pixel);
    }
  }
}
