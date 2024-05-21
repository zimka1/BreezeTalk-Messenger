cmake_minimum_required(VERSION 3.10)
project(BreezeTalk)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_TOOLCHAIN_FILE "/Users/zimka/vcpkg/scripts/buildsystems/vcpkg.cmake")


include_directories("/Users/zimka/vcpkg/installed/arm64-osx/include")
link_directories("/Users/zimka/vcpkg/installed/arm64-osx/lib")

add_executable(BreezeTalk main.cpp)

target_link_libraries(BreezeTalk PRIVATE uv z usockets)
