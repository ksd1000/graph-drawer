cmake_minimum_required(VERSION 3.10)
project(graph C)
if((NOT DEFINED CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL ""))
	set(CMAKE_BUILD_TYPE Release)
	set(CBT_ISDEF " (default)")
else()
	set(CBT_ISDEF "")
endif()
message("-- ${PROJECT_NAME}->CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}${CBT_ISDEF}")
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
