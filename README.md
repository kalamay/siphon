# siphon

Library of pull parsers for common network formats.

## Compile

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
make test
./bench-http
