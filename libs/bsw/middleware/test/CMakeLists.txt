add_executable(middlewareTest src/core/middleware_message_unittest.cpp
                              #src/core/middleware_proxy_base_unittest.cpp
                              #src/core/mock/logger_mock.cpp
                              src/queue/middleware_queue_unittest.cpp)

target_include_directories(
    middlewareTest
    INTERFACE $<INSTALL_INTERFACE:src>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(middlewareTest PRIVATE middleware gmock gtest_main)

gtest_discover_tests(middlewareTest PROPERTIES LABELS "middlewareTest")
