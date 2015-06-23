cmake_minimum_required(VERSION 3.0)

project(siphon C)

set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -Werror -pedantic")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")
set(CMAKE_SHARED_LINKER_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")

set(SIPHON_VERSION_MAJOR 0)
set(SIPHON_VERSION_MINOR 1)
set(SIPHON_VERSION_PATCH 1)
set(SIPHON_VERSION_STRING ${SIPHON_VERSION_MAJOR}.${SIPHON_VERSION_MINOR}.${SIPHON_VERSION_PATCH})

add_library(
	objects
	OBJECT
	lib/error.c
	lib/utf8.c
	lib/http.c
	lib/json.c
)

add_library(static STATIC $<TARGET_OBJECTS:objects>)
add_library(shared SHARED $<TARGET_OBJECTS:objects>)

set_target_properties(static shared PROPERTIES
	OUTPUT_NAME siphon
	VERSION ${SIPHON_VERSION_STRING}
	SOVERSION ${SIPHON_VERSION_STRING}
)

include_directories(BEFORE include)

add_executable(bench-http bench/http.c)
target_link_libraries(bench-http static)

add_executable(bench-json bench/json.c)
target_link_libraries(bench-json static)

enable_testing()

add_test(NAME utf8 COMMAND test-utf8)
add_executable(test-utf8 test/utf8.c)
target_link_libraries(test-utf8 static)

add_test(NAME http COMMAND test-http)
add_executable(test-http test/http.c)
target_link_libraries(test-http static)

add_test(NAME json COMMAND test-json)
add_executable(test-json test/json.c)
target_link_libraries(test-json static)

