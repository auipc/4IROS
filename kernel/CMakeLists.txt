cmake_minimum_required(VERSION 3.13)
project(4IROS C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=kernel -fPIE")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcmodel=kernel")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcmodel=kernel -fPIE")

set(SRCS
        arch/amd64/x64_preinit.cpp
        mem/malloc.cpp
        printk.cpp
	arch/amd64/asm/grib.S
	arch/amd64/asm/interrupt.S
#        gdt.h
#        gdt.cpp
#        idt.h
#        idt.cpp
#        PIC.cpp
#        PIC.h
#        mem/Paging.cpp
#		mem/Paging.h
#        assert.h
#        mem/PageFrameAllocator.cpp
#        mem/PageFrameAllocator.h
#        util/Bitmap.cpp
#        util/Bitmap.h
#		util/NBitmap.cpp
#		util/NBitmap.h
#		util/MemBinTree.h
#		util/MemBinTree.cpp
#	util/Spinlock.cpp
#		util/Vec.h
#        icxxabi.h
#		icxxabi.cpp
#        tasking/Process.cpp
#        tasking/Process.h
#		tasking/Scheduler.cpp
#		tasking/Scheduler.h
#		tasking/ELF.h
#		tasking/ELF.cpp
#		PIT.h
#		PIT.cpp
#		multiboot.h
#		vfs/vfs.cpp
#		vfs/vfs.h
#		vfs/block/ata.cpp
#		vfs/block/ata.h
#		vfs/fs/ext2.h
#		vfs/fs/ext2.cpp
#		vfs/stddev.h
#		vfs/stddev.cpp
#		Debug.h
#		Debug.cpp
#		Syscall.h
#		Syscall.cpp
#		driver/PS2Keyboard.h
#		driver/PS2Keyboard.cpp
)

add_executable(4IROS ${SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/libc)
target_link_libraries(4IROS ${LIBK_DIR}/libk.a)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
add_custom_command(TARGET 4IROS POST_BUILD COMMAND ld ARGS -r -b binary 4IROS -o arch/amd64/4IROS.o COMMENT Converting kernel image to object file)
