clear
rm -rf build/
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=./cmake/linux.cmake -GNinja .
cmake --build ./build