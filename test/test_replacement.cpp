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
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(0, cache_misses[0]);
}

TEST(replacement, rand_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0, 0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1, cache_misses[0]);
}

TEST(replacement, rand_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1, cache_misses[0]);
}

TEST(replacement, rand_replacement)
{
    auto m = 4u;
    auto A = replacement::RAND(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0),
        std::make_pair(4,0),
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_GE(cache_misses[0], 5u);
    ASSERT_LE(cache_misses[0], 9u);
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
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(0u, cache_misses[0]);
}

TEST(replacement, fifo_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
}

TEST(replacement, fifo_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
}

TEST(replacement, fifo_replacement)
{
    auto m = 4u;
    auto A = replacement::FIFO(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(0,0),
        std::make_pair(2,0),
        std::make_pair(0,0),
        std::make_pair(3,0),
        std::make_pair(0,0),
        std::make_pair(4,0),
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(6u, cache_misses[0]);
}

TEST(replacement, fifo_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = replacement::FIFO(
        m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0),
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
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
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(0u, cache_misses[0]);
}

TEST(replacement, lru_single_memory_reference_single_reference)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
}

TEST(replacement, lru_single_memory_reference_multiple_references)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0),
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
}

TEST(replacement, lru_replacement)
{
    auto m = 4u;
    auto A = replacement::LRU(m);
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(0,0),
        std::make_pair(2,0),
        std::make_pair(0,0),
        std::make_pair(3,0),
        std::make_pair(0,0),
        std::make_pair(4,0),
        std::make_pair(0,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(5u, cache_misses[0]);
}

TEST(replacement, lru_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = replacement::LRU(
        m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
    auto w = replacement::MemoryReferenceString{
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0),
        std::make_pair(0,0),
        std::make_pair(1,0),
        std::make_pair(2,0),
        std::make_pair(3,0)};
    replacement::numa_domain_type num_numa_domains = 1;
    std::vector<replacement::cache_miss_type> cache_misses =
        replacement::cost(A, w, num_numa_domains);
    ASSERT_EQ(1u, cache_misses[0]);
}

/*
 * Test counting the number of replacements for multiple threads
 * with a shared cache.
 */
TEST(replacement, lru_replacement_two_threads_shared_cache)
{
    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0),
             std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0)},
            {}};
        replacement::numa_domain_type num_numa_domains = 1;
        std::vector<std::vector<replacement::cache_miss_type>> cache_misses =
            replacement::cost(A, ws, num_numa_domains);
        ASSERT_EQ(1u, cache_misses[0][0]);
        ASSERT_EQ(0u, cache_misses[1][0]);
    }

    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0),
             std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0)},
            {std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0)}};
        replacement::numa_domain_type num_numa_domains = 1;
        std::vector<std::vector<replacement::cache_miss_type>> cache_misses =
            replacement::cost(A, ws, num_numa_domains);
        ASSERT_EQ(1u, cache_misses[0][0]);
        ASSERT_EQ(0u, cache_misses[1][0]);
    }

    {
        auto m = 4u;
        auto A = replacement::LRU(
            m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
        auto ws = std::vector<replacement::MemoryReferenceString>{
            {std::make_pair(0,0),
             std::make_pair(1,0),
             std::make_pair(2,0),
             std::make_pair(3,0),
             std::make_pair(2,0),
             std::make_pair(7,0),
             std::make_pair(2,0),
             std::make_pair(3,0)},
            {std::make_pair(4,0),
             std::make_pair(5,0),
             std::make_pair(6,0),
             std::make_pair(7,0),
             std::make_pair(6,0),
             std::make_pair(5,0),
             std::make_pair(6,0),
             std::make_pair(7,0)}};
        replacement::numa_domain_type num_numa_domains = 1;
        std::vector<std::vector<replacement::cache_miss_type>> cache_misses =
            replacement::cost(A, ws, num_numa_domains);
        ASSERT_EQ(3u, cache_misses[0][0]);
        ASSERT_EQ(6u, cache_misses[1][0]);
    }
}

/*
 * Test multiple NUMA domains.
 */
TEST(replacement, lru_replacement_numa_domains)
{
    auto m = 4u;
    auto A = replacement::LRU(
        m, std::vector<replacement::memory_reference_type>{0u, 1u, 2u});
    auto ws = std::vector<replacement::MemoryReferenceString>{
        {std::make_pair(0,0),
         std::make_pair(1,0),
         std::make_pair(2,0),
         std::make_pair(3,0),
         std::make_pair(2,0),
         std::make_pair(7,1),
         std::make_pair(2,0),
         std::make_pair(3,0)},
        {std::make_pair(4,0),
         std::make_pair(5,1),
         std::make_pair(6,1),
         std::make_pair(7,1),
         std::make_pair(6,0),
         std::make_pair(5,0),
         std::make_pair(6,0),
         std::make_pair(7,1)}};
    replacement::numa_domain_type num_numa_domains = 2;
    std::vector<std::vector<replacement::cache_miss_type>> cache_misses =
        replacement::cost(A, ws, num_numa_domains);
    ASSERT_EQ(3u, cache_misses[0][0]);
    ASSERT_EQ(0u, cache_misses[0][1]);
    ASSERT_EQ(2u, cache_misses[1][0]);
    ASSERT_EQ(4u, cache_misses[1][1]);
}
