#pragma once

#include "mandelbrot_common.hpp"

void write_pixels(const MandelBrotCalculationConfig &calculation_config,
                  const MandelBrotCalculationParams &calculation_params);
void write_thread_per_pixel(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params);

void write_baseline_iterations(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params);
bool compare_iterations_to_baseline(
    const MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params);