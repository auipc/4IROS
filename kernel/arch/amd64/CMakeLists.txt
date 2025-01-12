cmake_minimum_required(VERSION 3.13)
project(BOOT_SHIM C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

option(STRIP_BOOT_SHIM "Strip symbols from boot shim" ON)

set(SRCS
        asm/boot.S
	asm/64cont.S
        x64_start.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(BOOT_SHIM ${SRCS})

target_include_directories(BOOT_SHIM PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(BOOT_SHIM PRIVATE ${CMAKE_SOURCE_DIR}/libc)
target_link_libraries(BOOT_SHIM ${CMAKE_CURRENT_BINARY_DIR}/4IROS.o)
#target_link_libraries(BOOT_SHIM ${LIBK_DIR}/libk.a)
if (STRIP_BOOT_SHIM)
target_link_options(BOOT_SHIM PRIVATE LINKER:-s -T ${CMAKE_SOURCE_DIR}/kernel/arch/amd64/linker.ld -nostdlib -nodefaultlibs)
else()
target_link_options(BOOT_SHIM PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/arch/amd64/linker.ld -nostdlib -nodefaultlibs)
endif()
set_target_properties(BOOT_SHIM PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
add_dependencies(BOOT_SHIM 4IROS)
