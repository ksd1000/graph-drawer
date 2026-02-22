cmake_minimum_required(VERSION 3.10)
project(graph)
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG=1")
set(CMAKE_C_FLAGS_DEBUG "-Og -g -DNDEBUG=0 -fsanitize=address")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG=1")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG=1")
add_compile_options(-Wall -Werror)
add_library(graph STATIC
	graph.c
	texts/sbmp.c
	texts/text.c
)
