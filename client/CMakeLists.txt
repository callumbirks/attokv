add_executable(attokv_client
    src/client.cpp
    src/main.cpp
)

target_include_directories(attokv_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(attokv_client
    PRIVATE
        attokv_lib
)

target_compile_options(attokv_client
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
