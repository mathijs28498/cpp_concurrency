#include "mandelbrot_exec_calc.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <latch>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <spsc_queue.hpp>
#include <thread_pool.hpp>

#include "mandelbrot_common.hpp"

void exec_mb_thread_pool_1_wg_per_thread(
    MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params) {

    thread_pool::ThreadPool thread_pool =
        thread_pool::ThreadPool(calculation_config.amount_of_threads);
    (void)thread_pool.init();

    // Assumes 16 threads
    static constexpr std::size_t workgroup_size = IMAGE_SIZE / 16;
    static constexpr std::size_t amount_of_items = IMAGE_SIZE / workgroup_size;

    std::latch calculations_finished{amount_of_items};
    for (std::size_t iter{0}; iter < amount_of_items; iter++) {
        auto task = [&calculation_config, &calculation_params,
                     &calculations_finished, iter](unsigned int thread_idx) {
            std::size_t start_i = iter * workgroup_size;
            for (std::size_t i{start_i}; i < (start_i + workgroup_size); i++) {
                int x = i % IMAGE_WIDTH;
                int y = i / IMAGE_HEIGHT;

                calculation_config.iterations_per_pixel[i] =
                    calculate_mandelbrot_iterations(calculation_params, x, y);
                calculation_config.thread_per_pixel[i] = thread_idx;
            }

            calculations_finished.count_down();
        };

        while (!thread_pool.submit_task(task)) {
            std::this_thread::yield();
        };
    }

    calculations_finished.wait();
}

void exec_mb_thread_pool_32_wg_size(
    MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params) {

    thread_pool::ThreadPool thread_pool =
        thread_pool::ThreadPool(calculation_config.amount_of_threads);
    (void)thread_pool.init();

    static constexpr std::size_t workgroup_size = 32;
    static constexpr std::size_t amount_of_items = IMAGE_SIZE / workgroup_size;

    std::latch calculations_finished{amount_of_items};
    for (std::size_t iter{0}; iter < amount_of_items; iter++) {
        auto task = [&calculation_config, &calculation_params,
                     &calculations_finished, iter](unsigned int thread_idx) {
            std::size_t start_i = iter * workgroup_size;
            for (std::size_t i{start_i}; i < (start_i + workgroup_size); i++) {
                int x = i % IMAGE_WIDTH;
                int y = i / IMAGE_HEIGHT;

                calculation_config.iterations_per_pixel[i] =
                    calculate_mandelbrot_iterations(calculation_params, x, y);
                calculation_config.thread_per_pixel[i] = thread_idx;
            }

            calculations_finished.count_down();
        };

        while (!thread_pool.submit_task(task)) {
            std::this_thread::yield();
        };
    }

    calculations_finished.wait();
}