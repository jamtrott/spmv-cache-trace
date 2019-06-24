#include "cache-simulation/replacement.hpp"

#include <gtest/gtest.h>

/*
 * Test a replacement algorithm that replaces a random cache block.
 */
TEST(replacement, rand_empty)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{};
    ASSERT_EQ(0u, replacement::cost(A, w));
}

TEST(replacement, rand_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, rand_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, rand_replacement)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{0u, 1u, 2u, 3u, 4u, 0u, 1u, 2u, 3u};
    ASSERT_GE(replacement::cost(A, w), 5u);
    ASSERT_LE(replacement::cost(A, w), 9u);
}

/*
 * Test a replacement algorithm that replaces a cache block according to
 * the first-in-first-out policy.
 */
TEST(replacement, fifo_empty)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{};
    ASSERT_EQ(0u, replacement::cost(A, w));
}

TEST(replacement, fifo_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, fifo_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, fifo_replacement)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u, 0u};
    ASSERT_EQ(6u, replacement::cost(A, w));
}

TEST(replacement, fifo_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = replacement::FIFO(
        m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
    auto w = replacement::MemoryReferenceString{0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

/*
 * Test a replacement algorithm that replaces a cache block according to
 * the least recently used policy.
 */
TEST(replacement, lru_empty)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{};
    ASSERT_EQ(0u, replacement::cost(A, w));
}

TEST(replacement, lru_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, lru_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

TEST(replacement, lru_replacement)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u, 0u};
    ASSERT_EQ(5u, replacement::cost(A, w));
}

TEST(replacement, lru_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = replacement::LRU(
        m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
    auto w = replacement::MemoryReferenceString{0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u};
    ASSERT_EQ(1u, replacement::cost(A, w));
}

/*
 * Test counting the number of replacements for multiple threads
 * with a shared cache.
 */
TEST(memory_reference_replcament, lru_replacement_two_threads_shared_cache)
{
    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u},
            {}};
        auto cost = replacement::cost(A, ws);
        ASSERT_EQ(1u, cost[0]);
        ASSERT_EQ(0u, cost[1]);
    }

    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u},
            {0u, 1u, 2u, 3u}};
        auto cost = replacement::cost(A, ws);
        ASSERT_EQ(1u, cost[0]);
        ASSERT_EQ(0u, cost[1]);
    }

    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {0u, 1u, 2u, 3u, 2u, 7u, 2u, 3u},
            {4u, 5u, 6u, 7u, 6u, 5u, 6u, 7u}};
        auto cost = replacement::cost(A, ws);
        ASSERT_EQ(3u, cost[0]);
        ASSERT_EQ(6u, cost[1]);
    }
}
