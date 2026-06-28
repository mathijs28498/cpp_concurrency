add_executable(MainApp main.cpp)

set_property(TARGET MainApp PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(MainApp PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)

target_link_libraries(MainApp
    PRIVATE
    SPSCQueue
    ThreadPool
)