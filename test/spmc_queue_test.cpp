#include <cstdlib>
#include <list>
#include <gtest/gtest.h>
#include <spmc_queue.hpp>

constexpr std::size_t QUEUE_CAPACITY = 8;

template <typename T> class SPMCTest : public testing::Test {
  public:
    using List = std::list<T>;
    static T shared_;
    T value_;
};

using SPMCQueueTypes =
    ::testing::Types<spmc_queue::GlobalLockSPMCQueue<int, QUEUE_CAPACITY>>;
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

    int amount_of_items = 5;
    for (int i = 0; i < amount_of_items; i++) {
        EXPECT_TRUE(queue.enqueue(i));
        EXPECT_EQ(queue.size(), i + 1);
    }

    for (int i = 0; i < amount_of_items; i++) {
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

    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        EXPECT_TRUE(queue.enqueue(i));
        EXPECT_EQ(queue.size(), i + 1);
    }

    EXPECT_EQ(queue.size(), QUEUE_CAPACITY);

    for (int i = 0; i < 5; i++) {
        EXPECT_FALSE(queue.enqueue(i + QUEUE_CAPACITY));
        EXPECT_EQ(queue.size(), QUEUE_CAPACITY);
    }

    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        int value;
        bool successful_dequeue = queue.dequeue(value);
        EXPECT_TRUE(successful_dequeue);
        EXPECT_EQ(value, i);
        EXPECT_EQ(queue.size(), QUEUE_CAPACITY - i - 1);
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
    for (int i = 0; i < fill_and_drain_iterations; i++) {
        for (int j = 0; j < QUEUE_CAPACITY; j++) {
            EXPECT_TRUE(queue.enqueue(j));
            EXPECT_EQ(queue.size(), j + 1);
        }

        EXPECT_EQ(queue.size(), QUEUE_CAPACITY);

        EXPECT_FALSE(queue.enqueue(QUEUE_CAPACITY));

        for (int j = 0; j < QUEUE_CAPACITY; j++) {
            int value;
            bool successful_dequeue = queue.dequeue(value);
            EXPECT_TRUE(successful_dequeue);
            EXPECT_EQ(value, j);
            EXPECT_EQ(queue.size(), QUEUE_CAPACITY - j - 1);
        }

        EXPECT_EQ(queue.size(), 0);
        EXPECT_FALSE(queue.dequeue(dummy));
    }
}