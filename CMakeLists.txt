cmake_minimum_required(VERSION 3.19)
project(chip8
    VERSION 0.1
    DESCRIPTION "Chip8 Emulator"
    LANGUAGES C
)
set(CMAKE_BUILD_TYPE Debug)

find_package(SDL2 REQUIRED)

add_executable(chip8)
set_property(TARGET chip8 PROPERTY C_STANDARD 17)
target_sources(chip8 PUBLIC 
    src/main.c 
    src/emulator.c
)
target_link_libraries(chip8 ${SDL2_LIBRARIES}) 
target_include_directories(chip8 PRIVATE ${SDL2_INCLUDE_DIRS})