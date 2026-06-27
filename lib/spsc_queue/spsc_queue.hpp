#pragma once

#include <atomic>
#include <cstddef>

#define L1_CACHE_LINE_SIZE 64

namespace spsc_queue {
template <typename T, size_t N> class SPSCQueue {
  private:
    alignas(L1_CACHE_LINE_SIZE) T items[N];
    alignas(L1_CACHE_LINE_SIZE) std::atomic<size_t> tail = 0;
    alignas(L1_CACHE_LINE_SIZE) std::atomic<size_t> head = 0;

  public:
    SPSCQueue() {}

    bool enqueue(const T &item) {
        size_t currentHead = head.load(std::memory_order::relaxed);
        size_t currentTail = tail.load(std::memory_order::relaxed);
        size_t newTail = (currentTail + 1) % N;

        if (currentHead == newTail) {
            return false;
        }

        items[currentTail] = item;
        tail.store(newTail, std::memory_order::release);

        return true;
    }

    bool dequeue(T &item) {
        size_t currentHead = head.load(std::memory_order::relaxed);
        size_t currentTail = tail.load(std::memory_order::acquire);

        if (currentHead == currentTail) {
            return false;
        }

        item = items[currentHead];
        head.store((currentHead + 1) % N, std::memory_order::release);

        return true;
    }
};

} // namespace spsc_queue