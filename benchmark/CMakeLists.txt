add_library(attokv_benchmark_lib STATIC
    src/attokv_backend.cpp
    src/redis_backend.cpp
)

target_include_directories(attokv_benchmark_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(attokv_benchmark_lib
    PUBLIC
        attokv_client_lib
)

target_compile_options(attokv_benchmark_lib
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(attokv_benchmark
    src/main.cpp
)

target_link_libraries(attokv_benchmark
    PRIVATE
        attokv_benchmark_lib
)

target_compile_options(attokv_benchmark
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
