cmake_minimum_required(VERSION 3.13)
project(4IROS C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

set(SRCS
	kernel/boot.S
	kernel/main.cpp
	kernel/mem/malloc.cpp
	kernel/string.cpp
	kernel/printk.cpp
)

#file(GLOB CXX_SRCS kernel/main.cpp kernel/*.cpp kernel/*.h kernel/mem/*.cpp kernel/mem/*.h)
#file(GLOB ASM_SRCS kernel/*.S)

set(CMAKE_CXX_FLAGS "-ffreestanding -ggdb -O2 -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib")
add_executable(4IROS ${SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
