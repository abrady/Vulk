set -e
vcpkg/bootstrap-vcpkg.sh
vcpkg/vcpkg integrate install
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..