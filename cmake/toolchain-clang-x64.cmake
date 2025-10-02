# the name of the target operating system
set(CMAKE_SYSTEM_NAME 4IROS)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

set(CMAKE_CXX_FLAGS "--target=x86_64-pc-none-elf -march=x86-64 -m64 -nostdinc -nostdlib")
set(CMAKE_C_FLAGS "--target=x86_64-pc-none-elf -march=x86-64 -m64 -nostdinc -nostdlib")
set(CMAKE_LINKER_FLAGS "--target=x86_64-pc-none-elf -march=x86-64 -m64")
set(CMAKE_ASM_FLAGS "--target=x86_64-pc-none-elf -march=x86-64 -m64")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
