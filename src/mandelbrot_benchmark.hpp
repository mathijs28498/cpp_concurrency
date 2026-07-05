#pragma once

#include <array>
#include <assert.h>
#include <chrono>
#include <cstdlib>
#include <iostream>

#include <thread_pool.hpp>

#include "mandelbrot_common.hpp"
#include "mandelbrot_output.hpp"

constexpr std::array<MandelBrotCalculationParams, 9> calculation_params_list = {
    // constexpr std::array<MandelBrotCalculationParams, 1>
    // calculation_params_list = {
    {
        {-2.20, 0.80, -1.50, 1.50, 500, "00_overview"},
        {0.250, 0.300, -0.025, 0.025, 1000, "01_elephant_valley"},
        {-0.770, -0.730, 0.080, 0.120, 1000, "02_seahorse_valley"},
        {-0.1036, -0.0736, 0.6393, 0.6693, 1500, "03_triple_spiral"},
        {-1.795, -1.735, -0.030, 0.030, 1200, "04_airplane_island"},
        {-0.1692, -0.1492, 1.0217, 1.0417, 2500, "05_dendrite_island"},

        // The classic deep-zoom coordinate (-0.743643887037151 +
        // 0.131825904205330i)
        // at three depths: 4e-4, 4e-6, 2e-8 span.
        {-0.743843887037151, -0.743443887037151, 0.131625904205330,
         0.132025904205330, 2500, "06_spiral_wheels"},
        {-0.743645887037151, -0.743641887037151, 0.131823904205330,
         0.131827904205330, 8000, "07_wheels_within_wheels"},
        {-0.743643897037151, -0.743643877037151, 0.131825894205330,
         0.131825914205330, 15000, "08_embedded_julia"},
    },
};

static constexpr std::size_t BenchmarkCount = 30;

struct BenchmarkMetrics {
    long long median;
    long long min;
    long long max;
    long long iqr;
};

template <std::size_t Count> class Benchmark {
  private:
    std::array<long long, Count> benchmark_ns_results;
    std::chrono::steady_clock::time_point start_time;
    BenchmarkMetrics metrics;

  public:
    static constexpr std::size_t count = Count;

    inline BenchmarkMetrics get_metrics() { return metrics; }

    inline void start() { start_time = std::chrono::steady_clock::now(); }

    inline void stop(std::size_t iteration) {
        assert(iteration < count);

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time);
        benchmark_ns_results[iteration] = elapsed.count();
    }

    inline void calculate_metrics() {
        std::sort(benchmark_ns_results.begin(), benchmark_ns_results.end());
        static constexpr bool count_is_even = count % 2 == 0;
        static constexpr std::size_t half_count = count / 2;
        static constexpr std::size_t quarter_1_count = count / 4;
        static constexpr std::size_t quarter_3_count = (count * 3) / 4;

        long long median = count_is_even
                               ? (benchmark_ns_results[half_count - 1] +
                                  benchmark_ns_results[half_count]) /
                                     2
                               : benchmark_ns_results[half_count];

        long long iqr = benchmark_ns_results[quarter_3_count] -
                        benchmark_ns_results[quarter_1_count];

        metrics = {
            .median = median,
            .min = benchmark_ns_results[0],
            .max = benchmark_ns_results[count - 1],
            .iqr = iqr,
        };
    }
};

// TODO: This now does not interleave executions, which could give inaccurate
// results
template <void (*ExecFn)(MandelBrotCalculationConfig &calculation_config,
                         const MandelBrotCalculationParams &calculation_params)>
void benchmark_mandelbrot_fn(std::string name, unsigned int amount_of_threads) {
    MandelBrotCalculationConfig calculation_config{
        .amount_of_threads = amount_of_threads,
        .iterations_per_pixel = {},
        .smooth_per_pixel = {},
        .thread_per_pixel = {},
    };

    std::cout << "testing '" << name << "':" << std::endl;
    bool is_correct = true;
    for (const auto &calculation_params : calculation_params_list) {
        ExecFn(calculation_config, calculation_params);
        bool test_result = compare_iterations_to_baseline(calculation_config,
                                                          calculation_params);
        std::cout << "\tTesting '" << calculation_params.prefix
                  << "': " << (test_result ? "correct" : "incorrect")
                  << std::endl;
        if (!test_result) {
            is_correct = false;
        }
    }

    if (!is_correct) {
        std::cout << "ABORTING BENCHMARK -- incorrect algorithm" << std::endl;
        return;
    }

    std::cout << "\nStarting benchmark '" << name << "'\n" << std::endl;

    auto complete_benchmark_start = std::chrono::steady_clock::now();
    std::array<Benchmark<BenchmarkCount>, calculation_params_list.size()>
        benchmarks;
    for (std::size_t i{0}; i < calculation_params_list.size(); i++) {
        const MandelBrotCalculationParams &calculation_params =
            calculation_params_list[i];
        Benchmark<BenchmarkCount> &benchmark = benchmarks[i];

        for (std::size_t j{0}; j < benchmark.count; j++) {
            benchmark.start();
            ExecFn(calculation_config, calculation_params);
            benchmark.stop(j);
            // Sleep for a bit to let the threads cool down
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        benchmark.calculate_metrics();
        BenchmarkMetrics metrics = benchmark.get_metrics();

        std::cout << name << " - " << calculation_params.prefix
                  << ":\n\tmedian: " << metrics.median / 1000000.0
                  << "ms\n\tmin:    " << metrics.min / 1000000.0
                  << "ms\n\tmax:    " << metrics.max / 1000000.0
                  << "ms\n\tIQR:    " << metrics.iqr / 1000000.0 << "ms\n"
                  << std::endl;
    }

    auto complete_benchmark_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> complete_benchmark_duration =
        complete_benchmark_end - complete_benchmark_start;

    std::cout << "Finished benchmark '" << name << "'\n" << std::endl;
}