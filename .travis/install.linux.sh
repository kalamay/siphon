CMAKE_MINOR=3.5
CMAKE=cmake-${CMAKE_MINOR}.2-Linux-x86_64

sudo apt-get --assume-yes update
sudo apt-get --assume-yes install ragel valgrind
curl -O https://cmake.org/files/v${CMAKE_MINOR}/${CMAKE}.tar.gz
tar -xzf ${CMAKE}.tar.gz
cp -r ${CMAKE}/bin bin
./bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
./bin/cmake --build build
