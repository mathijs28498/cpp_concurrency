#pragma once

#include <atomic>
#include <cstddef>

namespace spsc_queue {
template <typename T, std::size_t N> class SPSCQueue {
  private:
    static constexpr std::size_t CacheLineSize = L1_CACHE_LINE_SIZE;

    alignas(CacheLineSize) T items[N];
    alignas(CacheLineSize) std::atomic<std::size_t> tail = 0;
    alignas(CacheLineSize) std::atomic<std::size_t> head = 0;

  public:
    SPSCQueue() {}

    bool enqueue(const T &item) {
        std::size_t currentHead = head.load(std::memory_order::relaxed);
        std::size_t currentTail = tail.load(std::memory_order::relaxed);
        std::size_t newTail = (currentTail + 1) % N;

        if (currentHead == newTail) {
            return false;
        }

        items[currentTail] = item;
        tail.store(newTail, std::memory_order::release);

        return true;
    }

    bool dequeue(T &item) {
        std::size_t currentHead = head.load(std::memory_order::relaxed);
        std::size_t currentTail = tail.load(std::memory_order::acquire);

        if (currentHead == currentTail) {
            return false;
        }

        item = items[currentHead];
        head.store((currentHead + 1) % N, std::memory_order::release);

        return true;
    }
};

} // namespace spsc_queue