add_library(attokv_server_lib STATIC
    src/command.cpp
    src/executor.cpp
    src/map.cpp
    src/server.cpp
    src/store.cpp
    src/string_arena.cpp
)

target_include_directories(attokv_server_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(attokv_server_lib
    PUBLIC
        attokv_lib
)

target_compile_options(attokv_server_lib
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(attokv_server
    src/main.cpp
)

target_link_libraries(attokv_server
    PRIVATE
        attokv_server_lib
)

target_compile_options(attokv_server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
