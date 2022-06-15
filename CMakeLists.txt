cmake_minimum_required(VERSION 3.19)
project(chip8
    VERSION 0.1
    DESCRIPTION "Gameboy Emulator"
    LANGUAGES C
)

add_executable(chip8)
set_property(TARGET chip8 PROPERTY C_STANDARD 17)
target_sources(chip8 PUBLIC src/main.c)
    