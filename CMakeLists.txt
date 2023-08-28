cmake_minimum_required(VERSION 3.13)
project(4IROS C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

set(SRCS
        kernel/arch/i386/asm/boot.S
        kernel/arch/i386/asm/interrupt.S
        kernel/main.cpp
        kernel/mem/malloc.h
        kernel/mem/malloc.cpp
        kernel/string.cpp
        kernel/printk.cpp
        kernel/stdarg.h
        kernel/gdt.h
        kernel/gdt.cpp
        kernel/idt.h
        kernel/idt.cpp
        kernel/arch/i386/IO.h
        kernel/PIC.cpp
        kernel/PIC.h
        kernel/mem/Paging.cpp
        kernel/mem/Paging.h
        kernel/assert.h
        kernel/mem/PageFrameAllocator.cpp
        kernel/mem/PageFrameAllocator.h
        kernel/util/Bitmap.cpp
        kernel/util/Bitmap.h
		kernel/util/Spinlock.cpp
		kernel/util/Spinlock.h
		kernel/util/Vec.h
        kernel/arch/i386/kernel.h
        kernel/icxxabi.h
		kernel/icxxabi.cpp
        kernel/tasking/Process.cpp
        kernel/tasking/Process.h
		kernel/tasking/Scheduler.cpp
		kernel/tasking/Scheduler.h
		kernel/tasking/ELF.h
		kernel/tasking/ELF.cpp
		kernel/test_program.h
)

set(CMAKE_CXX_FLAGS "-ffreestanding -ggdb -Os -Werror -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -mregparm=3 -I.")
add_executable(4IROS ${SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
