add_executable(MainApp)

target_sources(MainApp
    PRIVATE
    main.cpp
    exec_spsc_queue.cpp
    mandelbrot_output.cpp
    mandelbrot_exec_calc.cpp
    mandelbrot_common.cpp
    mandelbrot_benchmark.cpp

    PUBLIC
    FILE_SET HEADERS
)

set_property(TARGET MainApp PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(MainApp PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)

target_link_libraries(MainApp
    PRIVATE
    SPSCQueue
    SPMCQueue
    ThreadPool
)