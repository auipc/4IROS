cmake_minimum_required(VERSION 3.13)
project(4IROS C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

set(SRCS
        arch/i386/asm/boot.S
        arch/i386/asm/interrupt.S
        main.cpp
        mem/malloc.h
        mem/malloc.cpp
        printk.cpp
        gdt.h
        gdt.cpp
        idt.h
        idt.cpp
        PIC.cpp
        PIC.h
        mem/Paging.cpp
		mem/Paging.h
        assert.h
        mem/PageFrameAllocator.cpp
        mem/PageFrameAllocator.h
        util/Bitmap.cpp
        util/Bitmap.h
		util/NBitmap.cpp
		util/NBitmap.h
		util/MemBinTree.h
		util/MemBinTree.cpp
		util/Spinlock.cpp
		util/Spinlock.h
		util/Vec.h
        icxxabi.h
		icxxabi.cpp
        tasking/Process.cpp
        tasking/Process.h
		tasking/Scheduler.cpp
		tasking/Scheduler.h
		tasking/ELF.h
		tasking/ELF.cpp
		PIT.h
		PIT.cpp
		multiboot.h
		vfs/vfs.cpp
		vfs/vfs.h
		vfs/block/ata.cpp
		vfs/block/ata.h
		vfs/fs/ext2.h
		vfs/fs/ext2.cpp
		vfs/stddev.h
		vfs/stddev.cpp
		Debug.h
		Debug.cpp
		Syscall.h
		Syscall.cpp
		driver/PS2Keyboard.h
		driver/PS2Keyboard.cpp
)

add_executable(4IROS ${SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/)
target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/libc)
target_link_libraries(4IROS ${LIBK_DIR}/libk.a)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
