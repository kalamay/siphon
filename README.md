# siphon

Library of pull parsers for common network formats.

## Compile

### Debug build

```bash
cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
# or cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build/debug
cd build/debug
ctest --output-on-failure -D ExperimentalMemCheck
```

### Release build

```bash
cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
# or cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build/release
./build/release/bench-http
```
