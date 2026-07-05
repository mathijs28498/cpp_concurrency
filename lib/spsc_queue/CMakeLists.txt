add_library(SPSCQueue INTERFACE)

target_sources(SPSCQueue
    INTERFACE
    FILE_SET HEADERS
)

if(NOT DEFINED L1_CACHE_LINE_SIZE)
    set(L1_CACHE_LINE_SIZE "64")
endif()

target_compile_definitions(SPSCQueue INTERFACE "L1_CACHE_LINE_SIZE=${L1_CACHE_LINE_SIZE}")