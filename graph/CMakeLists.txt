cmake_minimum_required(VERSION 3.10)
project(graph C)
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
	set(CBT_ISDEF " (default)")
else()
	set(CBT_ISDEF "")
endif()
message("-- ${PROJECT_NAME}->CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}${CBT_ISDEF}")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
add_compile_options(-Wall)
add_library(graph
	graph.c
	texts/sbmp.c
	texts/text.c
)
