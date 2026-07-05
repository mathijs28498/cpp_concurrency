#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <mutex>
#include <new>

namespace spmc_queue {
template <typename ItemType, std::size_t Capacity> class GlobalLockSPMCQueue {
  private:
    static constexpr std::size_t CacheLineSize = L1_CACHE_LINE_SIZE;
    static constexpr std::size_t ItemAlignSize =
        std::max(CacheLineSize, alignof(ItemType));

    struct alignas(ItemAlignSize) PaddedItem {
        ItemType item;
    };

    alignas(CacheLineSize) std::array<PaddedItem, Capacity> items;

    alignas(CacheLineSize) mutable std::mutex queue_mutex;
    alignas(CacheLineSize) std::size_t head{0};
    alignas(CacheLineSize) std::size_t tail{0};

    std::size_t size_unlocked() { return tail - head; }

  public:
    GlobalLockSPMCQueue() = default;

    GlobalLockSPMCQueue(const GlobalLockSPMCQueue &other) {
        std::lock_guard<std::mutex> lock(other.queue_mutex);
        items = other.items;
        head = other.head;
        tail = other.tail;
    }

    bool enqueue(const ItemType &item) {
        std::lock_guard<std::mutex> lock(queue_mutex);

        if (size_unlocked() >= Capacity) {
            return false;
        }

        items[tail % Capacity] = PaddedItem{item};
        tail++;

        return true;
    }

    bool dequeue(ItemType &item) {
        std::lock_guard<std::mutex> lock(queue_mutex);

        if (head == tail) {
            return false;
        }

        item = items[head % Capacity].item;
        head++;
        return true;
    }

    std::size_t size() {
        std::lock_guard<std::mutex> lock{queue_mutex};
        return tail - head;
    }
};
} // namespace spmc_queue