cmake_minimum_required(VERSION 3.25.1)
    
# Required so neovim can find implementations of headers, i.e. go to implementation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# here we specify that the project is C language only, so the default
# C compiler on the system will be used
project(myprogram C)

add_executable(myprogram 
	main.c
	chunk.c
	memory.c
	debug.c
)

