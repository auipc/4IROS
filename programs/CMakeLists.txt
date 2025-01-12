set(PROGRAMS_SRCS
	init.c
	shell.c
	ls.c
	term.c
	cat.c
	farbview.c
	wump.c
	test.c
	less.c
	gol.c
	touch.c
	say.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE -fPIC -O3 -mred-zone")

foreach(PROGRAM IN LISTS PROGRAMS_SRCS)
	string(REPLACE ".c" "" PROGRAM_BIN ${PROGRAM})

	add_executable(${PROGRAM_BIN} ${PROGRAM})
	target_include_directories(${PROGRAM_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/)
	target_include_directories(${PROGRAM_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/include)
	target_include_directories(${PROGRAM_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/libc)
	target_link_libraries(${PROGRAM_BIN} ${LIBK_DIR}/libc.a)
	# is this correct?
	target_link_libraries(${PROGRAM_BIN} ${LIBK_DIR}/runtime/crt0.o)
	target_link_options(${PROGRAM_BIN} PRIVATE LINKER:-T ${CMAKE_SOURCE_DIR}/programs/linker.ld -nostdlib -nodefaultlibs)
	set_target_properties(${PROGRAM_BIN} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/programs/linker.ld)
	add_dependencies(${PROGRAM_BIN}  c)
endforeach()

