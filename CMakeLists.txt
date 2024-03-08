cmake_minimum_required(VERSION 3.13)
project(4I C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -nostdlib -fno-builtin -mregparm=3")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O0 -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -O0 -ggdb -Werror -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -mregparm=3")
add_subdirectory(libc)
add_subdirectory(kernel)
