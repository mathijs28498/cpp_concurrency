#include <iostream>

#include <thread_pool.hpp>

#include "mandelbrot_exec_calc.hpp"
#include "mandelbrot_benchmark.hpp"


int main() {
    unsigned int available_cpu_cores = thread_pool::get_available_cpu_cores();
    unsigned int amount_of_threads = 16;
    std::cout << "Utilizing " << amount_of_threads << " out of "
              << available_cpu_cores << " available cores" << std::endl;
    benchmark_mandelbrot_fn<exec_mb_thread_pool_32_wg_size>(
        "thread pool 32 workgroup size", amount_of_threads);

    benchmark_mandelbrot_fn<exec_mb_thread_pool_1_wg_per_thread>(
        "thread pool 1 workgroup per thread", amount_of_threads);

    return 0;
}