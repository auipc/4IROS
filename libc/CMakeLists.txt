set(LIBC_CRT0_SRCS
	crt0.c
)

set(LIBC_SRCS
	unistd.c
	stdio.c
	string.c
)

set(CMAKE_C_FLAGS "-ggdb -O0 -nostdlib -nostdinc")
add_library(crt0 STATIC ${LIBC_CRT0_SRCS})
set_target_properties(crt0 PROPERTIES PREFIX "" SUFFIX ".o")
add_library(c STATIC ${LIBC_SRCS})
target_include_directories(c PRIVATE .)
