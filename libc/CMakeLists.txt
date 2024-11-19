set(LIBC_CRT0_SRCS
	crt0.c
	unistd.c
)

set(LIBC_SRCS
	unistd.c
	stdio.c
	string.c
	sys/mman.c
	stdlib.c
	math.c
	asm/mathhelp.S
)

set(LIBK_SRCS
	stdlib.c
	string.c
	math.c
	asm/mathhelp.S
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fPIE")

add_library(crt0 STATIC ${LIBC_CRT0_SRCS})
set_target_properties(crt0 PROPERTIES PREFIX "" SUFFIX ".o")
target_include_directories(crt0 PRIVATE .)
add_library(c STATIC ${LIBC_SRCS})
target_include_directories(c PRIVATE .)
add_definitions(-DLIBK)
add_library(k STATIC ${LIBK_SRCS})
set(LIBK_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
target_include_directories(k PRIVATE .)
