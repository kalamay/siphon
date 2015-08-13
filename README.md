# siphon

Library of pull parsers for common network formats.

## Compile

### Release build

```bash
cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
# or cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build/release
./build/release/bench-http
```

### Debug build

```bash
cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
# or cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build/debug
cd build/debug
ctest --output-on-failure -D ExperimentalMemCheck
```

## Fuzzing

In addition to the test suite, the are a set of test tools that simply take
input on `stdin` and and report back success or failure. The fuzz tests use
these tools to attempt producing some input that cuases the tool to either
crash or hang.

Additionally, the memory used for these tests is allocated using the allocator
defined in lib/alloc.c. This allocator is very wasteful on memory, but protects
against buffer over-reads and over-writes. If either occur, the process will
crash and afl-fuzz will log the input that cuased the error condition.

Fuzzing is a *very* slow process. I typically run each fuzzer for 12-24 hours.

```bash
cmake -H. -Bbuild/fuzz -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/local/bin/afl-gcc
cmake --build build/fuzz

# set up the URI fuzz environment
mkdir -p build/fuzz-uri/{testcases,findings}
echo "http://user:pass@test.com/some/path.txt?a=b#test" > build/fuzz-uri/testcases/case01
# run these in different shells (best in tmux or screen)
afl-fuzz -i build/fuzz/testcases -o build/fuzz/findings -M fuzz01 ./test-uri-input
afl-fuzz -i build/fuzz/testcases -o build/fuzz/findings -S fuzz02 ./test-uri-input
afl-fuzz -i build/fuzz/testcases -o build/fuzz/findings -S fuzz03 ./test-uri-input
# run as many of these as you want, just make sure to update the -S parameter
# wait a long time
```
