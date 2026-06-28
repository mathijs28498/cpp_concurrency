#pragma once

#include <thread>
#include <vector>

namespace thread_pool {
unsigned int get_available_cpu_cores();

class ThreadPool {
  private:
    unsigned int thread_amount;
    std::vector<std::jthread> threads;

  public:
    ThreadPool(unsigned int thread_amount);
    int init();
};

} // namespace thread_pool
