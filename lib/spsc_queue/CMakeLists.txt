add_library(SPSCQueue INTERFACE)

target_sources(SPSCQueue
    PUBLIC
    FILE_SET HEADERS
)

set_property(TARGET SPSCQueue PROPERTY COMPILE_WARNING_AS_ERROR ON)
# target_compile_options(SPSCQueue PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)