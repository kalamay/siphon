brew install cmake
brew install ragel
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build
