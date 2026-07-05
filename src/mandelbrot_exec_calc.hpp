#pragma once

#include "mandelbrot_common.hpp"

void exec_mb_thread_pool_1_wg_per_thread(
    MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params);

void exec_mb_thread_pool_32_wg_size(
    MandelBrotCalculationConfig &calculation_config,
    const MandelBrotCalculationParams &calculation_params);
