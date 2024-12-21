cmake_minimum_required(VERSION 3.13)
project(4IROS C CXX ASM)
set(CMAKE_CXX_STANDARD 20)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=kernel -fPIE")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcmodel=kernel")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcmodel=kernel -fPIE")
if (UBSAN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

set(SRCS
        arch/amd64/x64_preinit.cpp
	driver/PS2Keyboard.cpp
	driver/VGAFramebuffer.cpp
	driver/BochsFramebuffer.cpp
	Syscall.cpp
	shedule/proc.cpp
	main.cpp
	ubsan.cpp
	vfs/vfs.cpp
	vfs/stddev.cpp
	vfs/block/ata.cpp
	vfs/fs/ext2.cpp
        mem/malloc.cpp
        mem/actualmalloc.cpp
        mem/Paging.cpp
	mem/PageFrameAllocator.cpp
	util/Bitmap.cpp
        printk.cpp
	arch/amd64/asm/grib.S
	arch/amd64/asm/interrupt.S
	arch/amd64/asm/syscall.S
	util/Spinlock.cpp
	icxxabi.cpp
	Debug.cpp
	prof.cpp
)

add_executable(4IROS ${SRCS})

target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(4IROS PRIVATE ${CMAKE_SOURCE_DIR}/libc)
target_link_libraries(4IROS ${LIBK_DIR}/libk.a)
target_link_options(4IROS PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -nostdlib -nodefaultlibs)
set_target_properties(4IROS PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/kernel/linker.ld)
add_custom_command(TARGET 4IROS POST_BUILD COMMAND ld ARGS -r -b binary 4IROS -o arch/amd64/4IROS.o COMMENT Converting kernel image to object file)
add_dependencies(4IROS c)
