set(LIBC_CRT0_SRCS
	crt0.c
	asm/crt0s.S
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
# FIXME C not CXX
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

add_library(crt0 STATIC ${LIBC_CRT0_SRCS})
set_target_properties(crt0 PROPERTIES PREFIX "" SUFFIX ".o")
target_include_directories(crt0 PRIVATE .)
target_include_directories(crt0 PRIVATE ..)
