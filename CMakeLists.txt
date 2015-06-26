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



find_program(RAGEL_COMMAND ragel)
if(NOT RAGEL_COMMAND)
	message(FATAL_ERROR "'ragel' not found")
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/uri_parser.c
	COMMAND ${RAGEL_COMMAND} -G2 -o
		${CMAKE_CURRENT_BINARY_DIR}/uri_parser.c
		${PROJECT_SOURCE_DIR}/lib/uri_parser.rl 
	DEPENDS
		${PROJECT_SOURCE_DIR}/lib/uri_parser.rl
)

add_library(
	objects
	OBJECT
	lib/error.c
	lib/crc.c
	lib/hash.c
	lib/utf8.c
	lib/http.c
	lib/json.c
	lib/path.c
	lib/uri.c
	${CMAKE_CURRENT_BINARY_DIR}/uri_parser.c
)

add_library(siphon-static STATIC $<TARGET_OBJECTS:objects>)
add_library(siphon-shared SHARED $<TARGET_OBJECTS:objects>)

set_target_properties(siphon-static siphon-shared PROPERTIES
	OUTPUT_NAME siphon
	VERSION ${SIPHON_VERSION_STRING}
	SOVERSION ${SIPHON_VERSION_STRING}
)

include_directories(BEFORE include)

install(TARGETS siphon-shared siphon-static
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(DIRECTORY include/siphon
	DESTINATION include
	FILES_MATCHING PATTERN "*.h")



add_executable(bench-http bench/http.c)
target_link_libraries(bench-http siphon-static)

add_executable(bench-json bench/json.c)
target_link_libraries(bench-json siphon-static)

add_executable(bench-crc bench/crc.c)
target_link_libraries(bench-crc siphon-static)



enable_testing()

set(MEMORYCHECK_TYPE Valgrind)
find_program(VALGRIND_COMMAND valgrind)
set(VALGRIND_COMMAND_OPTIONS "-q --tool=memcheck --leak-check=full --show-reachable=yes --num-callers=50")
if(APPLE)
	set(VALGRIND_COMMAND_OPTIONS "${VALGRIND_COMMAND_OPTIONS} --dsymutil=yes")
endif()
find_file(MEMORYCHECK_SUPPRESSIONS_FILE ${CMAKE_SYSTEM_NAME}.supp
	PATHS ${PROJECT_SOURCE_DIR}/test
	NO_DEFAULT_PATH)

include(CTest)

add_test(NAME crc COMMAND test-crc)
add_executable(test-crc test/crc.c)
target_link_libraries(test-crc siphon-static)

add_test(NAME hash COMMAND test-hash)
add_executable(test-hash test/hash.c)
target_link_libraries(test-hash siphon-static)

add_test(NAME utf8 COMMAND test-utf8)
add_executable(test-utf8 test/utf8.c)
target_link_libraries(test-utf8 siphon-static)

add_test(NAME http COMMAND test-http)
add_executable(test-http test/http.c)
target_link_libraries(test-http siphon-static)

add_test(NAME json COMMAND test-json)
add_executable(test-json test/json.c)
target_link_libraries(test-json siphon-static)

add_test(NAME path COMMAND test-path)
add_executable(test-path test/path.c)
target_link_libraries(test-path siphon-static)

add_test(NAME uri COMMAND test-uri)
add_executable(test-uri test/uri.c)
target_link_libraries(test-uri siphon-static)

