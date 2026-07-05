#include "mandelbrot_common.hpp"

unsigned int amount_of_threads = 0;

// TODO: Fix possible false sharing
std::array<int, IMAGE_SIZE> iterations_per_pixel;
std::array<std::thread::id, IMAGE_SIZE> thread_per_pixel;