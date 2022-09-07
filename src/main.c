#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#include "SDL_events.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
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

  Chip8Emulator emu;
  chip8_init_emulator(&emu);

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

  chip8_load_program(&emu, buffer, file_len);

  SDL_Event window_event;
  int running = 1;

  do {
    if (!running) {
      break;
    }
    chip8_run(&emu);
    while (SDL_PollEvent(&window_event)) {
      switch (window_event.type) {
      case SDL_QUIT:
        running = 0;
        break;
      }
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    chip8_render_display(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, &emu);
    // chip8_render_grid(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_RenderPresent(renderer);
  } while (1);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;

Error:
  if (program) {
    fclose(program);
  }
  return EXIT_FAILURE;
}
