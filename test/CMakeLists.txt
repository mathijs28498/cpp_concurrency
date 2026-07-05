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

set_property(TARGET testing PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(testing PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)

include(GoogleTest)
gtest_discover_tests(testing)