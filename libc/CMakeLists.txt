set(LIBC_SRCS
	errno.c
	unistd.c
	stdio.c
	string.c
	sys/mman.c
	stdlib.c
	math.c
	asm/mathhelp.S
	temporary_malloc.cpp
	gprof.c
	sys/time.c
	time.c
	alloca.c
	ctype.c
	signal.c
	dirent.c
)

set(LIBK_SRCS
	stdlib.c
	string.c
	math.c
	asm/mathhelp.S
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -fPIC -fPIE -O3")
# FIXME C not CXX
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fPIE -O3")

add_subdirectory(runtime)

add_library(c STATIC ${LIBC_SRCS})
target_include_directories(c PRIVATE .)
add_library(k STATIC ${LIBK_SRCS})
target_compile_definitions(k PUBLIC -DLIBK)
set(LIBK_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
target_include_directories(k PRIVATE .)
