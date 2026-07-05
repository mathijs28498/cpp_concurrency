#include <cstdlib>
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include <spmc_queue.hpp>

template <typename T> class SPMCTest : public testing::Test {
  public:
    T value_;
};

using SPMCQueueTypes =
    ::testing::Types<spmc_queue::GlobalLockSPMCQueue<int, 1>,
                     spmc_queue::GlobalLockSPMCQueue<int, 2>,
                     spmc_queue::GlobalLockSPMCQueue<int, 16>,
                     spmc_queue::GlobalLockSPMCQueue<int, 1024>>;
TYPED_TEST_SUITE(SPMCTest, SPMCQueueTypes);

TYPED_TEST(SPMCTest, HandleEmptyQueue) {
    TypeParam queue = this->value_;

    ASSERT_EQ(queue.size(), 0);
}

TYPED_TEST(SPMCTest, HandleEnqueueDequeueSingleThread) {
    TypeParam queue = this->value_;

    ASSERT_EQ(queue.size(), 0);
    int dummy;
    EXPECT_FALSE(queue.dequeue(dummy));

    int amount_of_items = std::min((int)queue.capacity - 1, 5);
    for (int i{0}; i < amount_of_items; i++) {
        EXPECT_TRUE(queue.enqueue(i));
        EXPECT_EQ(queue.size(), i + 1);
    }

    for (int i{0}; i < amount_of_items; i++) {
        int value;
        bool successful_dequeue = queue.dequeue(value);
        EXPECT_TRUE(successful_dequeue);
        EXPECT_EQ(value, i);
        EXPECT_EQ(queue.size(), amount_of_items - i - 1);
    }

    EXPECT_EQ(queue.size(), 0);
    EXPECT_FALSE(queue.dequeue(dummy));
}

TYPED_TEST(SPMCTest, HandleFullQueue) {
    TypeParam queue = this->value_;

    ASSERT_EQ(queue.size(), 0);
    int dummy;
    EXPECT_FALSE(queue.dequeue(dummy));

    for (int i{0}; i < (int)queue.capacity; i++) {
        EXPECT_TRUE(queue.enqueue(i));
        EXPECT_EQ(queue.size(), i + 1);
    }

    EXPECT_EQ(queue.size(), queue.capacity);

    for (int i{0}; i < 5; i++) {
        EXPECT_FALSE(queue.enqueue(i + queue.capacity));
        EXPECT_EQ(queue.size(), queue.capacity);
    }

    for (int i{0}; i < (int)queue.capacity; i++) {
        int value;
        bool successful_dequeue = queue.dequeue(value);
        EXPECT_TRUE(successful_dequeue);
        EXPECT_EQ(value, i);
        EXPECT_EQ(queue.size(), queue.capacity - i - 1);
    }

    EXPECT_EQ(queue.size(), 0);
    EXPECT_FALSE(queue.dequeue(dummy));
}

TYPED_TEST(SPMCTest, HandleRepeatedFillDrain) {
    TypeParam queue = this->value_;

    ASSERT_EQ(queue.size(), 0);
    int dummy;
    EXPECT_FALSE(queue.dequeue(dummy));

    constexpr int fill_and_drain_iterations = 100;
    for (int i{0}; i < fill_and_drain_iterations; i++) {
        for (int j{0}; j < (int)queue.capacity; j++) {
            EXPECT_TRUE(queue.enqueue(j));
            EXPECT_EQ(queue.size(), j + 1);
        }

        EXPECT_EQ(queue.size(), queue.capacity);

        EXPECT_FALSE(queue.enqueue(queue.capacity));

        for (int j{0}; j < (int)queue.capacity; j++) {
            int value;
            bool successful_dequeue = queue.dequeue(value);
            EXPECT_TRUE(successful_dequeue);
            EXPECT_EQ(value, j);
            EXPECT_EQ(queue.size(), queue.capacity - j - 1);
        }

        EXPECT_EQ(queue.size(), 0);
        EXPECT_FALSE(queue.dequeue(dummy));
    }
}

TYPED_TEST(SPMCTest, HandleSingleProducerMultipleConsumers) {
    TypeParam queue = this->value_;

    constexpr int num_consumers = 4;
    constexpr int amount_of_items = 100000;

    std::atomic<bool> start{false};
    std::atomic<bool> producer_done{false};

    std::vector<std::vector<int>> consumed(num_consumers);

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        for (int i{0}; i < amount_of_items; i++) {
            while (!queue.enqueue(i)) {
                std::this_thread::yield();
            }
        }

        producer_done.store(true, std::memory_order_release);
    });

    std::vector<std::thread> consumers;
    for (int i{0}; i < num_consumers; i++) {
        consumers.emplace_back([&, i] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            int value;
            while (true) {
                if (queue.dequeue(value)) {
                    consumed[i].push_back(value);
                } else if (producer_done.load(std::memory_order_acquire)) {
                    if (!queue.dequeue(value))
                        break;
                    consumed[i].push_back(value);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    start.store(true, std::memory_order_release);

    producer.join();
    for (std::thread &consumer : consumers) {
        consumer.join();
    }

    std::vector<int> all_consumed;
    for (std::vector<int> &v : consumed) {
        all_consumed.insert(all_consumed.end(), v.begin(), v.end());
    }

    ASSERT_EQ(all_consumed.size(), amount_of_items);
    std::sort(all_consumed.begin(), all_consumed.end());

    for (int i{0}; i < amount_of_items; i++) {
        ASSERT_EQ(all_consumed[i], i);
    }

    ASSERT_EQ(queue.size(), 0);
}