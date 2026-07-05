add_library(ThreadPool)

target_sources(ThreadPool
    PRIVATE
    thread_pool.cpp

    PUBLIC
    FILE_SET HEADERS
)

set_property(TARGET ThreadPool PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(ThreadPool PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)

target_link_libraries(ThreadPool
    PRIVATE
    SPMCQueue
)