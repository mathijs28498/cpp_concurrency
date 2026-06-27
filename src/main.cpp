#include <atomic>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

#include <spsc_queue.hpp>


int main() {
    spsc_queue::SPSCQueue<int, 1024> queue;

    std::thread producer([&]() {
        for (int i = 0; i < 100000; ++i) {
            while (!queue.enqueue(i)) {
                printf("Queue full, retrying: %d\n", i);
                std::this_thread::yield();
            }
            // if (!queue.enqueue(i)) {
            //     printf("Queue full, dropping: %d\n", i);
            //     std::this_thread::yield();
            // }
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