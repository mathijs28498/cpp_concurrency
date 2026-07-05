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
    spmc_queue::GlobalLockSPMCQueue<std::function<void(unsigned int)>, 1024> task_queue;
    std::vector<std::jthread> threads;

  public:
    ThreadPool(unsigned int thread_amount);
    int init();
    bool submit_task(std::function<void(unsigned int)> task);
};

} // namespace thread_pool
