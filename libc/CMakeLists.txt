set(LIBC_CRT0_SRCS
	crt0.c
)

set(LIBC_SRCS
	unistd.c
	stdio.c
	string.c
	sys/mman.c
	stdlib.c
)

set(LIBK_SRCS
	stdlib.c
	string.c
)

add_library(crt0 STATIC ${LIBC_CRT0_SRCS})
set_target_properties(crt0 PROPERTIES PREFIX "" SUFFIX ".o")
add_library(c STATIC ${LIBC_SRCS})
target_include_directories(c PRIVATE .)
add_definitions(-DLIBK)
add_library(k STATIC ${LIBK_SRCS})
set(LIBK_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
target_include_directories(k PRIVATE .)
