add_library(attokv_client_lib STATIC
    src/client.cpp
)

target_include_directories(attokv_client_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(attokv_client_lib
    PUBLIC
        attokv_lib
)

target_compile_options(attokv_client_lib
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(attokv_client
    src/main.cpp
)

target_link_libraries(attokv_client
    PRIVATE
        attokv_client_lib
)

target_compile_options(attokv_client
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
