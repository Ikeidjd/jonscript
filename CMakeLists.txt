cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Debug)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
# add_compile_definitions(DEBUG_PRINT_TOKENS)
# add_compile_definitions(DEBUG_PRINT_NODES_ONE)
# add_compile_definitions(DEBUG_PRINT_NODES_TWO)
# add_compile_definitions(DEBUG_PRINT_STRING_LITERALS)
# add_compile_definitions(DEBUG_DISASSEMBLE_CHUNK)
# add_compile_definitions(DEBUG_PRINT_FREED_OBJECTS)
# add_compile_definitions(DEBUG_TRACE_EXECUTION)
set(CMAKE_CXX_FLAGS "-g")

project(jon VERSION 1.0)

file(GLOB_RECURSE SRC_FILES src/*.c)
add_executable(jon ${SRC_FILES})
target_include_directories(jon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)