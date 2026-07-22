add_library(attokv_lib STATIC
    src/message.cpp
    src/util.cpp
)

target_include_directories(attokv_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(attokv_lib
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
