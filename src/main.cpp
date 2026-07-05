#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <spsc_queue.hpp>
#include <thread_pool.hpp>

void test_spsc_queue() {
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
}

void test_thread_pool() {
    unsigned int available_cpu_cores = thread_pool::get_available_cpu_cores();
    printf("available cpu cores: %u\n", available_cpu_cores);

    thread_pool::ThreadPool thread_pool =
        thread_pool::ThreadPool(available_cpu_cores - 1);
    (void)thread_pool.init();

    while (true) {
        (void)thread_pool.submit_task([]() {
            std::cout << "I'm in a thread: " << std::this_thread::get_id()
                      << std::endl;
        });
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
}

int main() {
    // test_spsc_queue();
    test_thread_pool();
    return 0;
}