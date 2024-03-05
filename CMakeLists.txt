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
		kernel/PIT.h
		kernel/PIT.cpp
		kernel/multiboot.h
		kernel/vfs/vfs.cpp
		kernel/vfs/vfs.h
		kernel/vfs/block/ata.cpp
		kernel/vfs/block/ata.h
		kernel/vfs/fs/ext2.h
		kernel/vfs/fs/ext2.cpp
		kernel/Debug.h
		kernel/Debug.cpp
		kernel/Syscall.h
		kernel/Syscall.cpp
)

set(CMAKE_C_FLAGS "-O0 -ggdb -nostdlib -nostdinc -fno-builtin")
set(CMAKE_ASM_FLAGS "-O0 -ggdb")
set(CMAKE_CXX_FLAGS "-ffreestanding -O0 -ggdb -Werror -Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -nostdinc -mregparm=3 -I.")
add_executable(4IROS ${SRCS})
add_subdirectory(libc)

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
