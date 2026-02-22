cmake_minimum_required(VERSION 3.10)
project(expr)
if((NOT DEFINED CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL ""))
	set(CMAKE_BUILD_TYPE Release)
	message("-- CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} (default)")
else()
	message("-- CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG=1")
set(CMAKE_C_FLAGS_DEBUG "-Og -g -DNDEBUG=0 -fsanitize=address")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG=1")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG=1")
add_compile_options(-Wall -Werror)
#set(LIBRARY_OUTPUT_PATH .)
add_library(expr STATIC
	expr_global.c
	expr_core.c
	expr_buffered.c
	expr_builtin.c
	expr_format.c
)
add_link_options(-lm)
add_executable(expr-version expr-version.c)
target_link_libraries(expr-version expr)
