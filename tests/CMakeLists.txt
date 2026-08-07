add_executable(attokv_map_test
    map_test.cpp
)

target_link_libraries(attokv_map_test
    PRIVATE
        attokv_server_lib
)

add_test(NAME attokv_map_test COMMAND attokv_map_test)

add_executable(attokv_redis_protocol_test
    redis_protocol_test.cpp
)

target_link_libraries(attokv_redis_protocol_test
    PRIVATE
        attokv_benchmark_lib
)

add_test(NAME attokv_redis_protocol_test COMMAND attokv_redis_protocol_test)

add_test(NAME attokv_client_help COMMAND attokv_client --help)
add_test(NAME attokv_server_help COMMAND attokv_server --help)
add_test(NAME attokv_benchmark_help COMMAND attokv_benchmark --help)

add_test(NAME attokv_client_invalid_port COMMAND attokv_client --port 0)
set_tests_properties(attokv_client_invalid_port PROPERTIES WILL_FAIL TRUE)
add_test(NAME attokv_server_invalid_port COMMAND attokv_server --port invalid)
set_tests_properties(attokv_server_invalid_port PROPERTIES WILL_FAIL TRUE)
add_test(NAME attokv_benchmark_missing_backend COMMAND attokv_benchmark --workload ping)
set_tests_properties(attokv_benchmark_missing_backend PROPERTIES WILL_FAIL TRUE)

add_test(
    NAME attokv_benchmark_integration
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/attokv_benchmark_integration.sh
            $<TARGET_FILE:attokv_server> $<TARGET_FILE:attokv_benchmark>
)
