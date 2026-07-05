#pragma once

#include <functional>
#include <spmc_queue.hpp>
#include <thread>
#include <vector>

namespace thread_pool {
unsigned int get_available_cpu_cores();

class ThreadPool {
  private:
    unsigned int thread_amount;
    std::vector<std::jthread> threads;
    spmc_queue::GlobalLockSPMCQueue<std::function<void()>, 1024> task_queue;

  public:
    ThreadPool(unsigned int thread_amount);
    int init();
    bool submit_task(std::function<void()> task);
};

} // namespace thread_pool
