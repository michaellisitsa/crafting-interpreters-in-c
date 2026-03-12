cmake_minimum_required(VERSION 3.25.1)
    
# Required so neovim can find implementations of headers, i.e. go to implementation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
# here we specify that the project is C language only, so the default
# C compiler on the system will be used
project(main C)

set(SOURCES
	main.c
	chunk.c
	memory.c
	debug.c
	value.c
	vm.c
	scanner.c
	compiler.c
	object.c
	table.c
)

add_executable(main ${SOURCES})
add_executable(maindbg ${SOURCES})
target_compile_definitions(maindbg PRIVATE "BUILD_B=1")
add_executable(maindump ${SOURCES})
target_compile_definitions(maindump PRIVATE "BUILD_C=1")

