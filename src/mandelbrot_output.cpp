#include "mandelbrot_output.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "mandelbrot_common.hpp"

template <std::size_t Size>
static inline void fill_result_pixel_data_iter(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params,
    std::array<unsigned char, Size> &result_pixel_data) {
    for (std::size_t i{0}; i < calculation_config.iterations_per_pixel.size();
         i++) {
        std::size_t pixel_index = i * 3;

        int iteration = calculation_config.iterations_per_pixel[i];
        if (iteration == calculation_params.max_iterations) {
            result_pixel_data[pixel_index] = 0;
            result_pixel_data[pixel_index + 1] = 0;
            result_pixel_data[pixel_index + 2] = 0;
            continue;
        }

        double t =
            static_cast<double>(iteration) / calculation_params.max_iterations;
        result_pixel_data[pixel_index] =
            static_cast<unsigned char>(9 * (1 - t) * t * t * t * 255);
        result_pixel_data[pixel_index + 1] =
            static_cast<unsigned char>(15 * (1 - t) * (1 - t) * t * t * 255);
        result_pixel_data[pixel_index + 2] = static_cast<unsigned char>(
            8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    }
}

struct Rgb {
    unsigned char r, g, b;
};

static constexpr std::size_t PALETTE_SIZE = 2048;

// Deep blue -> white -> orange -> near-black, wrapping back to blue so the
// cyclic mode has no seam. Retheme by editing the stop list; the mapping code
// below never changes.
static constexpr std::array<Rgb, PALETTE_SIZE> make_palette() {
    struct Stop {
        double pos, r, g, b;
    };
    constexpr Stop stops[]{
        {0.0, 0.0, 7.0, 100.0},      {0.16, 32.0, 107.0, 203.0},
        {0.42, 237.0, 255.0, 255.0}, {0.6425, 255.0, 170.0, 0.0},
        {0.8575, 0.0, 2.0, 0.0},     {1.0, 0.0, 7.0, 100.0},
    };

    std::array<Rgb, PALETTE_SIZE> lut{};
    for (std::size_t i{0}; i < PALETTE_SIZE; i++) {
        double pos = static_cast<double>(i) / PALETTE_SIZE;

        std::size_t s{0};
        while (pos > stops[s + 1].pos) {
            s++;
        }

        double f = (pos - stops[s].pos) / (stops[s + 1].pos - stops[s].pos);
        lut[i] = Rgb{
            static_cast<unsigned char>(stops[s].r +
                                       f * (stops[s + 1].r - stops[s].r) + 0.5),
            static_cast<unsigned char>(stops[s].g +
                                       f * (stops[s + 1].g - stops[s].g) + 0.5),
            static_cast<unsigned char>(stops[s].b +
                                       f * (stops[s + 1].b - stops[s].b) + 0.5),
        };
    }
    return lut;
}

static constexpr auto palette = make_palette();

static inline Rgb color_from_t(double t) {
    std::size_t index =
        std::min(static_cast<std::size_t>(t * PALETTE_SIZE), PALETTE_SIZE - 1);
    return palette[index];
}

template <std::size_t Size>
static inline void fill_result_pixel_data_smooth(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params,
    std::array<unsigned char, Size> &result_pixel_data) {
    std::size_t buckets =
        static_cast<std::size_t>(calculation_params.max_iterations) + 1;

    std::vector<std::uint32_t> histogram(buckets, 0);
    std::size_t escaped_count{0};
    for (double smooth : calculation_config.smooth_per_pixel) {
        if (smooth < 0.0) {
            continue;
        }
        std::size_t bucket =
            std::min(static_cast<std::size_t>(smooth), buckets - 1);
        histogram[bucket]++;
        escaped_count++;
    }

    // cumulative[k] = number of escaped pixels with floor(smooth) < k
    std::vector<std::uint32_t> cumulative(buckets + 1, 0);
    for (std::size_t k{0}; k < buckets; k++) {
        cumulative[k + 1] = cumulative[k] + histogram[k];
    }

    double rank_to_t =
        escaped_count ? 1.0 / static_cast<double>(escaped_count) : 0.0;

    for (std::size_t i{0}; i < calculation_config.smooth_per_pixel.size();
         i++) {
        std::size_t pixel_index = i * 3;

        double smooth = calculation_config.smooth_per_pixel[i];
        if (smooth < 0.0) {
            result_pixel_data[pixel_index] = 0;
            result_pixel_data[pixel_index + 1] = 0;
            result_pixel_data[pixel_index + 2] = 0;
            continue;
        }

        std::size_t bucket =
            std::min(static_cast<std::size_t>(smooth), buckets - 1);
        double fraction = smooth - static_cast<double>(bucket);
        double rank = cumulative[bucket] + fraction * histogram[bucket];

        Rgb color = color_from_t(rank * rank_to_t);
        result_pixel_data[pixel_index] = color.r;
        result_pixel_data[pixel_index + 1] = color.g;
        result_pixel_data[pixel_index + 2] = color.b;
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
    const MandelBrotCalculationConfig &calculation_config,
    std::array<unsigned char, Size> &threads_per_pixel_data) {

    std::unordered_map<std::thread::id, std::array<unsigned char, 3>> color_map;
    std::vector<std::array<unsigned char, 3>> thread_colors;
    thread_colors.reserve(calculation_config.amount_of_threads);
    for (std::size_t i{0}; i < calculation_config.amount_of_threads; i++) {
        double hue =
            360.0 / (double)calculation_config.amount_of_threads * (double)i;
        std::array<unsigned char, 3> color = hsv_to_rgb(hue, 1, 1);
        thread_colors.push_back(std::move(color));
    }

    color_map.reserve(calculation_config.amount_of_threads);

    for (std::size_t i{0}; i < calculation_config.thread_per_pixel.size();
         i++) {
        std::array<unsigned char, 3> color =
            thread_colors[calculation_config.thread_per_pixel[i]];

        threads_per_pixel_data[i * 3] = color[0];
        threads_per_pixel_data[i * 3 + 1] = color[1];
        threads_per_pixel_data[i * 3 + 2] = color[2];
    }
}

static std::string ppm_header_data = "P6\n" + std::to_string(IMAGE_WIDTH) + " " +
                          std::to_string(IMAGE_HEIGHT) + " 255\n";

void write_pixels(const MandelBrotCalculationConfig &calculation_config,
                  const MandelBrotCalculationParams &calculation_params) {

    {
        std::ofstream result_iter_ppm("../../output/" +
                                      std::string(calculation_params.prefix) +
                                      "_result_iter.ppm");

        result_iter_ppm.write(ppm_header_data.data(), ppm_header_data.size());

        std::array<unsigned char, IMAGE_SIZE * 3> result_iter_pixel_data;
        fill_result_pixel_data_iter(calculation_config, calculation_params,
                                    result_iter_pixel_data);
        result_iter_ppm.write(
            reinterpret_cast<const char *>(result_iter_pixel_data.data()),
            result_iter_pixel_data.size());

        result_iter_ppm.close();
    }

    {
        std::ofstream result_smooth_ppm("../../output/" +
                                        std::string(calculation_params.prefix) +
                                        "_result_smooth.ppm");

        result_smooth_ppm.write(ppm_header_data.data(), ppm_header_data.size());

        std::array<unsigned char, IMAGE_SIZE * 3> result_smooth_pixel_data;
        fill_result_pixel_data_smooth(calculation_config, calculation_params,
                                      result_smooth_pixel_data);
        result_smooth_ppm.write(
            reinterpret_cast<const char *>(result_smooth_pixel_data.data()),
            result_smooth_pixel_data.size());

        result_smooth_ppm.close();
    }
}

void write_thread_per_pixel(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params) {

    {
        std::ofstream threads_per_pixel_ppm(
            "../../output/" + std::string(calculation_params.prefix) +
            "_threads_per_pixel.ppm");

        threads_per_pixel_ppm.write(ppm_header_data.data(), ppm_header_data.size());

        std::array<unsigned char, IMAGE_SIZE * 3> threads_per_pixel_data;

        fill_threads_per_pixel_data(calculation_config, threads_per_pixel_data);

        threads_per_pixel_ppm.write(
            reinterpret_cast<const char *>(threads_per_pixel_data.data()),
            threads_per_pixel_data.size());

        threads_per_pixel_ppm.close();
    }
}

void write_baseline_iterations(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params) {
    std::ofstream out_file("../../baseline/" +
                               std::string(calculation_params.prefix) +
                               "_iterations_baseline.bin",
                           std::ios::binary);

    out_file.write(reinterpret_cast<const char *>(
                       calculation_config.iterations_per_pixel.data()),
                   calculation_config.iterations_per_pixel.size() *
                       sizeof(calculation_config.iterations_per_pixel[0]));

    out_file.close();
}

bool compare_iterations_to_baseline(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params) {
    std::array<int, IMAGE_SIZE> baseline_iterations_per_pixel;

    std::ifstream in_file("../../baseline/" +
                              std::string(calculation_params.prefix) +
                              "_iterations_baseline.bin",
                          std::ios::binary);

    in_file.read(reinterpret_cast<char *>(baseline_iterations_per_pixel.data()),
                 baseline_iterations_per_pixel.size() *
                     sizeof(baseline_iterations_per_pixel[0]));

    in_file.close();

    return baseline_iterations_per_pixel ==
           calculation_config.iterations_per_pixel;
}