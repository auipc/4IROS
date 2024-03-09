set(PROGRAMS_SRCS
	init.c
)

foreach(PROGRAM IN LISTS PROGRAMS_SRCS)
	string(REPLACE ".c" "" PROGRAM_BIN ${PROGRAM})

	add_executable(${PROGRAM_BIN} ${PROGRAM})
	target_include_directories(${PROGRAM_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/)
	target_include_directories(${PROGRAM_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/libc)
	target_link_libraries(${PROGRAM_BIN} ${LIBK_DIR}/libc.a)
	# is this correct?
	target_link_libraries(${PROGRAM_BIN} ${LIBK_DIR}/crt0.o)
endforeach()
