#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <latch>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <spsc_queue.hpp>
#include <thread_pool.hpp>

static constexpr std::size_t IMAGE_WIDTH = 512;
static constexpr std::size_t IMAGE_HEIGHT = 512;
static constexpr std::size_t IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;
static unsigned int amount_of_threads = 0;

// TODO: Fix possible false sharing
static std::array<int, IMAGE_SIZE> iterations_per_pixel;
static std::array<std::thread::id, IMAGE_SIZE> thread_per_pixel;

struct MandelBrotCalculationData {
    double re_min, re_max;
    double im_min, im_max;
    int max_iterations;
    std::string_view prefix;
};

template <std::size_t Size>
static inline void
fill_result_pixel_data(std::array<unsigned char, Size> &result_pixel_data,
                       const MandelBrotCalculationData &calculation_data) {
    for (std::size_t i{0}; i < iterations_per_pixel.size(); i++) {
        std::size_t pixel_index = i * 3;

        int iteration = iterations_per_pixel[i];
        if (iteration == calculation_data.max_iterations) {
            result_pixel_data[pixel_index] = 0;
            result_pixel_data[pixel_index + 1] = 0;
            result_pixel_data[pixel_index + 2] = 0;
            continue;
        }

        double t =
            static_cast<double>(iteration) / calculation_data.max_iterations;
        result_pixel_data[pixel_index] =
            static_cast<unsigned char>(9 * (1 - t) * t * t * t * 255);
        result_pixel_data[pixel_index + 1] =
            static_cast<unsigned char>(15 * (1 - t) * (1 - t) * t * t * 255);
        result_pixel_data[pixel_index + 2] = static_cast<unsigned char>(
            8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    }
}

std::array<unsigned char, 3> hsv_to_rgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1 - std::abs(std::fmod(h / 60.0, 2) - 1));
    double m = v - c;
    double r, g, b;
    if (h < 60) {
        r = c;
        g = x;
        b = 0;
    } else if (h < 120) {
        r = x;
        g = c;
        b = 0;
    } else if (h < 180) {
        r = 0;
        g = c;
        b = x;
    } else if (h < 240) {
        r = 0;
        g = x;
        b = c;
    } else if (h < 300) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }
    return {static_cast<unsigned char>((r + m) * 255),
            static_cast<unsigned char>((g + m) * 255),
            static_cast<unsigned char>((b + m) * 255)};
}

template <std::size_t Size>
static inline void fill_threads_per_pixel_data(
    std::array<unsigned char, Size> &threads_per_pixel_data) {

    std::unordered_map<std::thread::id, std::array<unsigned char, 3>> color_map;
    color_map.reserve(amount_of_threads);

    for (std::size_t i{0}; i < thread_per_pixel.size(); i++) {
        std::array<unsigned char, 3> color;
        auto element = color_map.find(thread_per_pixel[i]);
        if (element == color_map.end()) {
            double hue =
                360.0 / (double)amount_of_threads * (double)color_map.size();
            color = hsv_to_rgb(hue, 1, 1);
            color_map.emplace(thread_per_pixel[i], color);
        } else {
            color = element->second;
        }

        threads_per_pixel_data[i * 3] = color[0];
        threads_per_pixel_data[i * 3 + 1] = color[1];
        threads_per_pixel_data[i * 3 + 2] = color[2];
    }
}

void write_pixels(const MandelBrotCalculationData &calculation_data) {
    std::ofstream result_ppm(
        "../../output/" + std::string(calculation_data.prefix) + "_result.ppm");

    std::string header_data = "P6\n" + std::to_string(IMAGE_WIDTH) + " " +
                              std::to_string(IMAGE_HEIGHT) + " 255\n";
    result_ppm.write(header_data.data(), header_data.size());

    std::array<unsigned char, IMAGE_SIZE * 3> result_pixel_data;

    fill_result_pixel_data(result_pixel_data, calculation_data);

    result_ppm.write(reinterpret_cast<const char *>(result_pixel_data.data()),
                     result_pixel_data.size());

    result_ppm.close();

    std::ofstream threads_per_pixel_ppm("../../output/" +
                                        std::string(calculation_data.prefix) +
                                        "_threads_per_pixel.ppm");

    threads_per_pixel_ppm.write(header_data.data(), header_data.size());

    std::array<unsigned char, IMAGE_SIZE * 3> threads_per_pixel_data;

    fill_threads_per_pixel_data(threads_per_pixel_data);

    threads_per_pixel_ppm.write(
        reinterpret_cast<const char *>(threads_per_pixel_data.data()),
        threads_per_pixel_data.size());

    threads_per_pixel_ppm.close();
}

static inline int calculate_mandelbrot_x_y(
    const struct MandelBrotCalculationData &calculation_data, int x, int y) {

    // Map pixel (x, y) to a point c in the complex plane
    double c_re = calculation_data.re_min +
                  (static_cast<double>(x) / IMAGE_WIDTH) *
                      (calculation_data.re_max - calculation_data.re_min);
    double c_im = calculation_data.im_min +
                  (static_cast<double>(y) / IMAGE_HEIGHT) *
                      (calculation_data.im_max - calculation_data.im_min);

    // Iterate z = z^2 + c, starting from z = 0
    double z_re = 0.0;
    double z_im = 0.0;
    int iteration = 0;

    while (z_re * z_re + z_im * z_im <= 4.0 &&
           iteration < calculation_data.max_iterations) {
        double new_re = z_re * z_re - z_im * z_im + c_re;
        double new_im = 2.0 * z_re * z_im + c_im;
        z_re = new_re;
        z_im = new_im;
        iteration++;
    }

    return iteration;
}

void exec_thread_pool(const MandelBrotCalculationData &calculation_data) {

    thread_pool::ThreadPool thread_pool =
        thread_pool::ThreadPool(amount_of_threads);
    (void)thread_pool.init();

    static constexpr std::size_t WORKGROUP_SIZE = 32;

    // Mandelbrot viewport bounds (tweak to zoom/pan)

    static constexpr std::size_t amount_of_items = IMAGE_SIZE / WORKGROUP_SIZE;
    std::latch calculations_finished{amount_of_items};
    for (std::size_t iter{0}; iter < amount_of_items; iter++) {
        while (!thread_pool.submit_task([iter, &calculation_data,
                                         &calculations_finished]() {
            std::size_t start_i = iter * WORKGROUP_SIZE;
            for (std::size_t i{start_i}; i < (start_i + WORKGROUP_SIZE); i++) {
                int x = i % IMAGE_WIDTH;
                int y = i / IMAGE_HEIGHT;

                iterations_per_pixel[i] =
                    calculate_mandelbrot_x_y(calculation_data, x, y);
                thread_per_pixel[i] = std::this_thread::get_id();
            }

            calculations_finished.count_down();
        })) {
            std::this_thread::yield();
        };
    }

    calculations_finished.wait();
}

static constexpr std::array<MandelBrotCalculationData, 9> calculation_data_list{
    {
        // The postcard shot. Interior-heavy: middle rows cost ~max_iterations,
        // edge rows are cheap. Worst case for static row partitioning.
        {-2.20, 0.80, -1.50, 1.50, 500, "00_overview"},

        // Elephant Valley: spirals marching into the cusp at c = 0.25.
        {0.250, 0.300, -0.025, 0.025, 1000, "01_elephant_valley"},

        // Seahorse Valley: the cleft between the cardioid and the period-2
        // disk.
        {-0.770, -0.730, 0.080, 0.120, 1000, "02_seahorse_valley"},

        // Triple Spiral Valley, upper region of the set.
        {-0.1036, -0.0736, 0.6393, 0.6693, 1500, "03_triple_spiral"},

        // The period-3 minibrot on the needle, complete with its own antenna.
        {-1.795, -1.735, -0.030, 0.030, 1200, "04_airplane_island"},

        // Largest island hanging off the top dendrite, surrounded by lightning.
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
    }};

int main() {
    unsigned int available_cpu_cores = thread_pool::get_available_cpu_cores();
    printf("available cpu cores: %u\n", available_cpu_cores);
    amount_of_threads = available_cpu_cores - 1;

    for (const auto &calculation_data : calculation_data_list) {
        exec_thread_pool(calculation_data);
        write_pixels(calculation_data);
    }
    return 0;
}