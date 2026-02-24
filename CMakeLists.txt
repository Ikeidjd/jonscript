cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Debug)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
set(CMAKE_CXX_FLAGS "-g")

project(jon VERSION 1.0)

file(GLOB_RECURSE SRC_FILES src/*.c)
add_executable(jon ${SRC_FILES})
target_include_directories(jon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)