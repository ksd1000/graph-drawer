cmake_minimum_required(VERSION 3.10)
project(expr C)
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
	set(CBT_ISDEF " (default)")
else()
	set(CBT_ISDEF "")
endif()
message("-- ${PROJECT_NAME}->CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}${CBT_ISDEF}")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -DEXPR_DEBUG=1")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
add_compile_options(-Wall -Wextra -Werror)
#set(LIBRARY_OUTPUT_PATH .)
add_library(expr
	expr-global.c
	expr-builtin.c
	expr-core.c
	expr-buffered.c
	expr-format.c
	expr-alloc.c
	expr-internal.c
)
link_libraries(expr m)
add_executable(expr-version expr-version.c)
if(EXPR_CHECK)
	add_link_options(-fsanitize=address)
	add_executable(expr-check check.c)
endif()
