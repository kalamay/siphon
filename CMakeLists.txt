cmake_minimum_required(VERSION 2.8)

project(siphon C)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -Werror -pedantic -D_BSD_SOURCE -D_GNU_SOURCE -fPIC -fvisibility=hidden")
set(CMAKE_C_FLAGS_DEBUG "-g -DSP_ALLOC_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
SET(CMAKE_MACOSX_RPATH ON)

find_program(VALGRIND_COMMAND valgrind)

if(VALGRIND_COMMAND AND CMAKE_BUILD_TYPE MATCHES Debug)
	get_filename_component(VALGRIND_INCLUDE ${VALGRIND_COMMAND} DIRECTORY)
	get_filename_component(VALGRIND_INCLUDE ${VALGRIND_INCLUDE} DIRECTORY)
	set(VALGRIND_INCLUDE "${VALGRIND_INCLUDE}/include")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSP_USE_VALGRIND")
	include_directories(BEFORE ${VALGRIND_INCLUDE})
endif()

set(SIPHON_VER_MAJOR 0)
set(SIPHON_VER_MINOR 2)
set(SIPHON_VER_PATCH 0)
set(SIPHON_VER_STRING ${SIPHON_VER_MAJOR}.${SIPHON_VER_MINOR}.${SIPHON_VER_PATCH})



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

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cache_parser.c
	COMMAND ${RAGEL_COMMAND} -G2 -o
		${CMAKE_CURRENT_BINARY_DIR}/cache_parser.c
		${PROJECT_SOURCE_DIR}/lib/cache_parser.rl 
	DEPENDS
		${PROJECT_SOURCE_DIR}/lib/cache_parser.rl
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.h
	"#ifndef SIPHON_VER_H\n"
	"#define SIPHON_VER_H\n\n"
	"#define SP_VER_MAJOR ${SIPHON_VER_MAJOR}\n"
	"#define SP_VER_MINOR ${SIPHON_VER_MINOR}\n"
	"#define SP_VER_PATCH ${SIPHON_VER_PATCH}\n"
	"#define SP_VER (SP_VER_MAJOR*1000000 + SP_VER_MINOR*1000 + SP_VER_PATCH)\n"
	"#define SP_VER_STRING \"siphon ${SIPHON_VER_MAJOR}.${SIPHON_VER_MINOR}.${SIPHON_VER_PATCH}\"\n"
	"\n#endif\n"
)

add_library(siphon OBJECT
	lib/error.c
	lib/clock.c
	lib/common.c
	lib/seed.c
	lib/fmt.c
	lib/crc.c
	lib/hash.c
	lib/bloom.c
	lib/map.c
	lib/vec.c
	lib/trie.c
	lib/rand.c
	lib/utf8.c
	lib/http.c
	lib/json.c
	lib/msgpack.c
	lib/path.c
	lib/uri.c
	lib/alloc.c
	lib/line.c
	${CMAKE_CURRENT_BINARY_DIR}/uri_parser.c
	${CMAKE_CURRENT_BINARY_DIR}/cache_parser.c
)

add_library(siphon-static STATIC $<TARGET_OBJECTS:siphon>)
add_library(siphon-shared SHARED $<TARGET_OBJECTS:siphon>)

set_target_properties(siphon-static siphon-shared PROPERTIES
	OUTPUT_NAME siphon
	VERSION ${SIPHON_VER_STRING}
	SOVERSION ${SIPHON_VER_STRING}
)

include_directories(BEFORE include ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS siphon-static siphon-shared
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
	DESTINATION include/siphon)

install(DIRECTORY include/siphon
	DESTINATION include
	FILES_MATCHING PATTERN "*.h")

install(DIRECTORY man/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3
	FILES_MATCHING PATTERN "*.3")



add_executable(bench-http bench/http.c)
target_link_libraries(bench-http siphon-static m)

add_executable(bench-json bench/json.c)
target_link_libraries(bench-json siphon-static m)

add_executable(bench-crc bench/crc.c)
target_link_libraries(bench-crc siphon-static m)



enable_testing()

set(MEMORYCHECK_TYPE Valgrind)
set(VALGRIND_COMMAND_OPTIONS "-q --tool=memcheck --leak-check=full --show-reachable=yes --num-callers=50 --error-exitcode=2")
if(APPLE)
	set(VALGRIND_COMMAND_OPTIONS "${VALGRIND_COMMAND_OPTIONS} --dsymutil=yes")
endif()
find_file(MEMORYCHECK_SUPPRESSIONS_FILE ${CMAKE_SYSTEM_NAME}.supp
	PATHS ${PROJECT_SOURCE_DIR}/test
	NO_DEFAULT_PATH)

include(CTest)

add_test(NAME crc COMMAND test-crc)
add_executable(test-crc test/crc.c)
target_link_libraries(test-crc siphon-static m)

add_test(NAME hash COMMAND test-hash)
add_executable(test-hash test/hash.c)
target_link_libraries(test-hash siphon-static m)

add_test(NAME utf8 COMMAND test-utf8)
add_executable(test-utf8 test/utf8.c)
target_link_libraries(test-utf8 siphon-static m)

add_test(NAME http COMMAND test-http)
add_executable(test-http test/http.c)
target_link_libraries(test-http siphon-static m)
add_executable(test-http-input test/http-input.c)
target_link_libraries(test-http-input siphon-static m)

add_test(NAME json COMMAND test-json)
add_executable(test-json test/json.c)
target_link_libraries(test-json siphon-static m)
add_executable(test-json-input test/json-input.c)
target_link_libraries(test-json-input siphon-static m)

add_test(NAME msgpack COMMAND test-msgpack)
add_executable(test-msgpack test/msgpack.c)
target_link_libraries(test-msgpack siphon-static m)
add_executable(test-msgpack-input test/msgpack-input.c)
target_link_libraries(test-msgpack-input siphon-static m)

add_test(NAME path COMMAND test-path)
add_executable(test-path test/path.c)
target_link_libraries(test-path siphon-static m)

add_test(NAME uri COMMAND test-uri)
add_executable(test-uri test/uri.c)
target_link_libraries(test-uri siphon-static m)
add_executable(test-uri-input test/uri-input.c)
target_link_libraries(test-uri-input siphon-static m)

add_test(NAME bloom COMMAND test-bloom)
add_executable(test-bloom test/bloom.c)
target_link_libraries(test-bloom siphon-static m)

add_test(NAME cache COMMAND test-cache)
add_executable(test-cache test/cache.c)
target_link_libraries(test-cache siphon-static m)

add_test(NAME line COMMAND test-line)
add_executable(test-line test/line.c)
target_link_libraries(test-line siphon-static m)

add_test(NAME error COMMAND test-error)
add_executable(test-error test/error.c)
target_link_libraries(test-error siphon-static m)

add_test(NAME clock COMMAND test-clock)
add_executable(test-clock test/clock.c)
target_link_libraries(test-clock siphon-static m)

add_test(NAME map COMMAND test-map)
add_executable(test-map test/map.c)
target_link_libraries(test-map siphon-static m)

add_test(NAME vec COMMAND test-vec)
add_executable(test-vec test/vec.c)
target_link_libraries(test-vec siphon-static m)

add_test(NAME trie COMMAND test-trie)
add_executable(test-trie test/trie.c)
target_link_libraries(test-trie siphon-static m)

