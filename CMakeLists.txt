cmake_minimum_required(VERSION 3.13)
project(4I C CXX ASM)

option(SSE4 "Enables SSE4" OFF)
option(UBSAN "Enables Clang's UBSAN" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ggdb -gdwarf-2 -Wall -nostdlib -nostdinc -mno-red-zone -mregparm=3")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -O3 -Wunused-result -ggdb -gdwarf-2 -Werror -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -nostdinc -mno-red-zone -mregparm=3 -Wno-unused-function -mstackrealign")

if (SSE4)
	add_definitions(-DOS_SSE4)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4")
endif()

add_subdirectory(libc)
add_subdirectory(kernel)
add_subdirectory(kernel/arch/amd64)
add_subdirectory(programs)
