add_executable(testing)

target_sources(testing
    PRIVATE
    spmc_queue_test.cpp
)

target_link_libraries(
    testing
    GTest::gtest_main
    SPMCQueue
)

include(GoogleTest)
gtest_discover_tests(testing)