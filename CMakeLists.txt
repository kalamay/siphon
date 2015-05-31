cmake_minimum_required(VERSION 3.0)

project(netparse C)

set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -Werror -pedantic")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

add_library(
	netparse
	include/netparse/http.h lib/http.c
	include/netparse/error.h lib/error.c
)

include_directories(BEFORE include)

add_executable(bench-http bench/http.c)
target_link_libraries (bench-http netparse)

enable_testing()

add_test(NAME http COMMAND test-http)
add_executable(test-http test/http.c)
target_link_libraries (test-http netparse)

