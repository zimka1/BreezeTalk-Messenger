find_package(PostgreSQL REQUIRED)


cmake_minimum_required(VERSION 3.10)
project(BreezeTalk)

set(CMAKE_CXX_STANDARD 17)

# Укажите путь к toolchain файлу vcpkg
set(CMAKE_TOOLCHAIN_FILE "/Users/zimka/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Найдите и подключите PostgreSQL
find_package(PostgreSQL REQUIRED)

# Включите необходимые директории
include_directories("/Users/zimka/vcpkg/installed/arm64-osx/include")
link_directories("/Users/zimka/vcpkg/installed/arm64-osx/lib")

# Добавьте исполняемый файл
add_executable(BreezeTalk server/src/main.cpp
        server/src/message.cpp
        server/src/websocket.cpp
        server/src/database.cpp
        server/src/globals.cpp
)

# Свяжите с libpq (PostgreSQL)
target_link_libraries(BreezeTalk PRIVATE PostgreSQL::PostgreSQL uv z usockets)