cmake_minimum_required(VERSION 3.10)
project(BreezeTalk)

set(CMAKE_CXX_STANDARD 17)

# Укажите путь к toolchain файлу vcpkg
set(CMAKE_TOOLCHAIN_FILE "/Users/zimka/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Найдите и подключите необходимые библиотеки
find_package(SQLite3 REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(ZLIB REQUIRED)

# Включите необходимые директории
include_directories("/Users/zimka/vcpkg/installed/arm64-osx/include")
link_directories("/Users/zimka/vcpkg/installed/arm64-osx/lib")

# Добавьте исполняемый файл
add_executable(BreezeTalk main.cpp
)

# Свяжите исполняемый файл с библиотеками
target_link_libraries(BreezeTalk PRIVATE SQLite::SQLite3 SQLiteCpp ZLIB::ZLIB uv z usockets)
