clear
rm -rf build/
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=./linux.cmake -GNinja .
cmake --build ./build