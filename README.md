# siphon

Library of pull parsers for common network formats.

## Compile

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
# or cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
cmake --build build --target test
./build/bench-http
```
