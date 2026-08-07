cmake_minimum_required(VERSION 3.20)

project(attokv LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(benchmark)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
