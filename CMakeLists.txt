cmake_minimum_required(VERSION 3.13)  # CMake version check
project(4IROS C CXX ASM)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard

file(GLOB CXX_SRCS kernel/main.cpp kernel/*.cpp kernel/*.h kernel/mem/*.cpp kernel/mem/*.h)
file(GLOB ASM_SRCS kernel/*.s)

set(CMAKE_CXX_FLAGS "-ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib")
add_executable(4IROS ${CXX_SRCS} ${ASM_SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
