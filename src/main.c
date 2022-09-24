#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#include "SDL_events.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "emulator.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 640;

int main(int argc, char **argv) {
  SDL_Window *window;
  SDL_Renderer *renderer;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                            SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created!\n");
    exit(EXIT_FAILURE);
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Could not initialize renderer\n");
    exit(EXIT_FAILURE);
  }

  Chip8Emulator emulator;
  chip8_init_emulator(&emulator);

  // check that the user provides a file
  if (argc < 2) {
    puts("Please point the emulator to a program file");
    return EXIT_FAILURE;
  }
  FILE *program;
  program = fopen(argv[1], "rb");
  if (!program) {
    printf("Failed to open file: %s", argv[1]);
    goto Error;
  }
  fseek(program, 0, SEEK_END);
  long file_len = ftell(program);
  if (file_len > MAX_PROGRAM_SIZE || file_len == -1) {
    puts("The program provided may be too large");
    goto Error;
  }
  rewind(program);
  uint8_t buffer[4096];
  fread(buffer, file_len, 1, program);
  fclose(program);

  chip8_load_program(&emulator, buffer, file_len);

  SDL_Event window_event;
  int running = 1;

  uint64_t timer = SDL_GetTicks64();

  while(running) {
    uint64_t current_time = SDL_GetTicks64();
    uint64_t delta_time = current_time - timer;
    timer = current_time;
    
    while (SDL_PollEvent(&window_event)) {
      switch (window_event.type) {
      case SDL_QUIT:
        running = 0;
        break;
      case SDL_KEYDOWN:
      switch (window_event.key.keysym.sym) {
        case SDLK_1:
        emulator.inputs[1] = 1;
        break;
        
        case SDLK_2:
        emulator.inputs[2] = 1;
        break;

        case SDLK_3:
        emulator.inputs[3] = 1;
        break;

        case SDLK_4:
        emulator.inputs[0xc] = 1;
        break;

        case SDLK_q:
        emulator.inputs[0x4] = 1;
        break;

        case SDLK_w:
        emulator.inputs[0x5] = 1;
        break;

        case SDLK_e:
        emulator.inputs[0x6] = 1;
        break;

        case SDLK_r:
        emulator.inputs[0xd] = 1;
        break;

        case SDLK_a:
        emulator.inputs[0x7] = 1;
        break;

        case SDLK_s:
        emulator.inputs[0x8] = 1;
        break;

        case SDLK_d:
        emulator.inputs[0x9] = 1;
        break;

        case SDLK_f:
        emulator.inputs[0xe] = 1;
        break;

        case SDLK_z:
        emulator.inputs[0xa] = 1;
        break;

        case SDLK_x:
        emulator.inputs[0x0] = 1;
        break;

        case SDLK_c:
        emulator.inputs[0xb] = 1;
        break;

        case SDLK_v:
        emulator.inputs[0xf] = 1;
        break;
      }
      break;

      case SDL_KEYUP:
      switch (window_event.key.keysym.sym) {
        case SDLK_1:
        emulator.inputs[1] = 0;
        break;
        
        case SDLK_2:
        emulator.inputs[2] = 0;
        break;

        case SDLK_3:
        emulator.inputs[3] = 0;
        break;

        case SDLK_4:
        emulator.inputs[0xc] = 0;
        break;

        case SDLK_q:
        emulator.inputs[0x4] = 0;
        break;

        case SDLK_w:
        emulator.inputs[0x5] = 0;
        break;

        case SDLK_e:
        emulator.inputs[0x6] = 0;
        break;

        case SDLK_r:
        emulator.inputs[0xd] = 0;
        break;

        case SDLK_a:
        emulator.inputs[0x7] = 0;
        break;

        case SDLK_s:
        emulator.inputs[0x8] = 0;
        break;

        case SDLK_d:
        emulator.inputs[0x9] = 0;
        break;

        case SDLK_f:
        emulator.inputs[0xe] = 0;
        break;

        case SDLK_z:
        emulator.inputs[0xa] = 0;
        break;

        case SDLK_x:
        emulator.inputs[0x0] = 0;
        break;

        case SDLK_c:
        emulator.inputs[0xb] = 0;
        break;

        case SDLK_v:
        emulator.inputs[0xf] = 0;
        break;
      }
      

  }
    }
    chip8_run(&emulator, delta_time, window_event);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    chip8_render_display(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, &emulator);
    // chip8_render_grid(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_RenderPresent(renderer);
  } 

  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;

Error:
  if (program) {
    fclose(program);
  }
  return EXIT_FAILURE;
}
