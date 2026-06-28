#include "thread_pool.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace thread_pool {
unsigned int get_available_cpu_cores() {
    return std::thread::hardware_concurrency();
}

ThreadPool::ThreadPool(unsigned int thread_amount)
    : thread_amount(thread_amount) {};

int ThreadPool::init() {
    threads.reserve(thread_amount);

    auto worker = [](unsigned int thread_idx) {
        srand(static_cast<unsigned int>(time(nullptr)) + thread_idx);

        auto sleep_duration = std::chrono::milliseconds(500 + (rand() % 2000));
        std::this_thread::sleep_for(sleep_duration);
        std::cout << "From worker: " << std::this_thread::get_id()
                  << " idx: " << thread_idx
                  << " slept for: " << sleep_duration.count() << "ms"
                  << std::endl;
    };

    for (unsigned int i = 0; i < thread_amount; i++) {
        threads.emplace_back(std::jthread(worker, i));
    }

    return 0;
}
} // namespace thread_pool