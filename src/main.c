#include <stdio.h>
#include <stdlib.h>

#include "emulator.h"

int main(int argc, char **argv) {
    Emulator emu;
    init_emulator(&emu);

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

    load_program(&emu, buffer, file_len);

    do {
        run(&emu);
    } while (1);

    return EXIT_SUCCESS;

    Error:
    if (program) {
        fclose(program);
    }
    return EXIT_FAILURE;
}