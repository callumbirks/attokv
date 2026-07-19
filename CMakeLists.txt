cmake_minimum_required(VERSION 3.20)

project(kvcli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(kvcli
    src/main.cpp
    src/store.cpp
    src/command.cpp
)

target_include_directories(kvcli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(kvcli
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
