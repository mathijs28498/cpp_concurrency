#include <atomic>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

#define L1_CACHE_LINE_SIZE 64

// TODO: Make it work with static memory
template <typename T, size_t N> class SPSCQueue {
  private:
    size_t capacity;
    alignas(L1_CACHE_LINE_SIZE) T items[N];
    alignas(L1_CACHE_LINE_SIZE) std::atomic<size_t> tail = 0;
    alignas(L1_CACHE_LINE_SIZE) std::atomic<size_t> head = 0;

  public:
    explicit SPSCQueue() {}

    bool enqueue(const T &item) {
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t currentHead = head.load(std::memory_order_relaxed);

        if (currentTail - currentHead >= N) {
            return false;
        }

        items[currentTail % N] = item;
        tail.store(currentTail + 1, std::memory_order_release);
        return true;
    }

    bool dequeue(T &item) {
        size_t currentTail = tail.load(std::memory_order_acquire);
        size_t currentHead = head.load(std::memory_order_relaxed);

        if (currentHead == currentTail) {
            return false;
        }

        item = items[currentHead % N];
        head.store(currentHead + 1, std::memory_order_release);
        return true;
    }
};

int main() {
    SPSCQueue<int, 1024> queue;

    std::thread producer([&]() {
        for (int i = 0; i < 100000; ++i) {
            // while (!queue.enqueue(i)) {
            //     printf("Queue full, retrying: %d\n", i);
            //     std::this_thread::yield();
            // }
            if (!queue.enqueue(i)) {
                printf("Queue full, dropping: %d\n", i);
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        int val;
        for (int i = 0; i < 100000; ++i) {
            while (!queue.dequeue(val)) {
                std::this_thread::yield();
            }
            printf("found val: %d, iter: %d\n", val, i);
        }
        printf("Successfully processed 100,000 elements!\n");
    });

    producer.join();
    consumer.join();
    return 0;
}