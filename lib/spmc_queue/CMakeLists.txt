add_library(SPMCQueue)

target_sources(SPMCQueue
    PRIVATE
    spmc_queue.cpp

    PUBLIC
    FILE_SET HEADERS
)

set_property(TARGET SPMCQueue PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(SPMCQueue PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)

if(NOT DEFINED L1_CACHE_LINE_SIZE)
    set(L1_CACHE_LINE_SIZE "64")
endif()

target_compile_definitions(SPMCQueue PUBLIC "L1_CACHE_LINE_SIZE=${L1_CACHE_LINE_SIZE}")