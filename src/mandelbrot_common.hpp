#pragma once

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <thread>

constexpr std::size_t IMAGE_WIDTH = 512;
constexpr std::size_t IMAGE_HEIGHT = 512;
constexpr std::size_t IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;
constexpr double INTERIOR = -1.;

struct MandelBrotCalculationParams {
    double re_min, re_max;
    double im_min, im_max;
    int max_iterations;
    std::string_view prefix;
};

struct MandelBrotCalculationConfig {
    unsigned int amount_of_threads;

    // TODO: Fix possible false sharing
    std::array<int, IMAGE_SIZE> iterations_per_pixel;
    std::array<double, IMAGE_SIZE> smooth_per_pixel;
    std::array<unsigned int, IMAGE_SIZE> thread_per_pixel;
};

static inline int calculate_mandelbrot_iterations(
    const struct MandelBrotCalculationParams &calculation_params, int x,
    int y) {

    // Map pixel (x, y) to a point c in the complex plane
    double c_re = calculation_params.re_min +
                  (static_cast<double>(x) / IMAGE_WIDTH) *
                      (calculation_params.re_max - calculation_params.re_min);
    double c_im = calculation_params.im_min +
                  (static_cast<double>(y) / IMAGE_HEIGHT) *
                      (calculation_params.im_max - calculation_params.im_min);

    // Iterate z = z^2 + c, starting from z = 0
    double z_re = 0.0;
    double z_im = 0.0;
    int iteration = 0;

    while (z_re * z_re + z_im * z_im <= 4.0 &&
           iteration < calculation_params.max_iterations) {
        double new_re = z_re * z_re - z_im * z_im + c_re;
        double new_im = 2.0 * z_re * z_im + c_im;
        z_re = new_re;
        z_im = new_im;
        iteration++;
    }

    return iteration;
}

static inline double calculate_mandelbrot_smooth(
    const struct MandelBrotCalculationParams &calculation_params, int x,
    int y) {

    // Map pixel (x, y) to a point c in the complex plane
    double c_re = calculation_params.re_min +
                  (static_cast<double>(x) / IMAGE_WIDTH) *
                      (calculation_params.re_max - calculation_params.re_min);
    double c_im = calculation_params.im_min +
                  (static_cast<double>(y) / IMAGE_HEIGHT) *
                      (calculation_params.im_max - calculation_params.im_min);

    // Radius 256 instead of 2: the smooth estimate below needs |z| well past
    // the escape circle to be accurate. Costs ~3 extra iterations per
    // escaping pixel.
    static constexpr double ESCAPE_RADIUS_SQUARED = 65536.0; // 256^2

    // Iterate z = z^2 + c, starting from z = 0. The squares are cached so the
    // loop condition reuses them and |z|^2 is available after the loop.
    double z_re = 0.0;
    double z_im = 0.0;
    double z_re_sq = 0.0;
    double z_im_sq = 0.0;
    int iteration = 0;

    while (z_re_sq + z_im_sq <= ESCAPE_RADIUS_SQUARED &&
           iteration < calculation_params.max_iterations) {
        z_im = 2.0 * z_re * z_im + c_im; // uses the old z_re: order matters
        z_re = z_re_sq - z_im_sq + c_re;
        z_re_sq = z_re * z_re;
        z_im_sq = z_im * z_im;
        iteration++;
    }

    // Interior is classified by the radius test, not iteration count: a point
    // that escapes on the very last iteration exits the loop with
    // iteration == max_iterations but is still an escaped point.
    if (z_re_sq + z_im_sq <= ESCAPE_RADIUS_SQUARED) {
        return INTERIOR;
    }

    // Smooth iteration count: each extra iteration roughly squares |z|, so
    // log2(ln|z|) grows by 1 per iteration; subtracting it recovers the
    // fractional overshoot past the bailout.
    double log_z_abs = 0.5 * std::log(z_re_sq + z_im_sq); // ln|z|
    return std::max(0.0, iteration + 1.0 - std::log2(log_z_abs));
}