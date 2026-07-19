add_executable(attokv_server
    src/main.cpp
    src/command.cpp
    src/executor.cpp
    src/message.cpp
    src/server.cpp
    src/store.cpp
)

target_include_directories(attokv_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(attokv_server
    PRIVATE
        attokv_lib
)

target_compile_options(attokv_server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
