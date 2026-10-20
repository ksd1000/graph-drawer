cmake_minimum_required(VERSION 3.13)
project(expr C)
include(CTest)
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
add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wnull-dereference)
#set(LIBRARY_OUTPUT_PATH .)
if(ASAN)
	add_compile_options(-fsanitize=address,undefined)
	add_link_options(-fsanitize=address,undefined)
endif()
add_library(expr
	expr-core.c
	expr-symext.c
	expr-buffered.c
	expr-format.c
	expr-alloc.c
	expr-internal.c
)
target_compile_options(expr PRIVATE -ffreestanding)
add_library(expr-lc
	expr-global.c
	expr-builtin.c
)
target_link_libraries(expr-lc PUBLIC expr)
link_libraries(expr expr-lc m)
add_executable(expr-version expr-version.c)
add_executable(expr-check expr-check.c)

install(TARGETS expr-lc expr expr-version
	EXPORT expr
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib 
	INCLUDES DESTINATION include
)

install(FILES expr.h
	DESTINATION include
)

enable_testing()
add_test(NAME expr-check COMMAND expr-check)
set_tests_properties(expr-check PROPERTIES TIMEOUT 120)
