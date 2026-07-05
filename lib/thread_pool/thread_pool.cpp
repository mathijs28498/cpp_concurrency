#include "thread_pool.hpp"

#include <chrono>
#include <iostream>
#include <spmc_queue.hpp>
#include <thread>

namespace thread_pool {
unsigned int get_available_cpu_cores() {
    return std::thread::hardware_concurrency();
}

ThreadPool::ThreadPool(unsigned int thread_amount)
    : thread_amount(thread_amount) {};

int ThreadPool::init() {
    threads.reserve(thread_amount);

    auto *task_queue_ptr = &task_queue;

    // TODO: Add a stop condition
    auto worker = [task_queue_ptr](std::stop_token stop_token,
                                   unsigned int thread_idx) {
        (void)thread_idx;
        std::function<void(unsigned int)> task;
        while (!stop_token.stop_requested()) {
            if (task_queue_ptr->dequeue(task)) {
                if (task) {
                    task(thread_idx);
                } else {
                    printf("Something went terribly wrong!\n");
                }
            } else {
                std::this_thread::yield();
            }
        }
    };

    for (unsigned int i = 0; i < thread_amount; i++) {
        threads.emplace_back(std::jthread(worker, i));
    }

    return 0;
}

bool ThreadPool::submit_task(std::function<void(unsigned int)> task) {
    return task_queue.enqueue(std::move(task));
}
} // namespace thread_pool