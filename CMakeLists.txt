cmake_minimum_required(VERSION 3.13)  # CMake version check
project(4IROS C CXX ASM)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard

file(GLOB CXX_SRCS src/*.cpp)
file(GLOB ASM_SRCS src/*.s)

set(CMAKE_CXX_FLAGS "-ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib")
add_executable(4IROS ${CXX_SRCS} ${ASM_SRCS})

target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/src/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/src/linker.ld)
