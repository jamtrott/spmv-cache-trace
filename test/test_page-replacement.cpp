#include "cache-simulation/page-replacement.hpp"

#include <gtest/gtest.h>

/*
 * Test a page replacement algorithm that replaces a random page.
 */
TEST(page_replacement, rand_empty)
{
    auto m = 4u;
    auto A = page_replacement::RAND(m);
    auto w = page_replacement::PageReferenceString{};
    ASSERT_EQ(0u, page_replacement::cost(A, w));
}

TEST(page_replacement, rand_single_page_single_reference)
{
    auto m = 4u;
    auto A = page_replacement::RAND(m);
    auto w = page_replacement::PageReferenceString{0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, rand_single_page_multiple_references)
{
    auto m = 4u;
    auto A = page_replacement::RAND(m);
    auto w = page_replacement::PageReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, rand_replacement)
{
    auto m = 4u;
    auto A = page_replacement::RAND(m);
    auto w = page_replacement::PageReferenceString{0u, 1u, 2u, 3u, 4u, 0u, 1u, 2u, 3u};
    ASSERT_GE(page_replacement::cost(A, w), 5u);
    ASSERT_LE(page_replacement::cost(A, w), 9u);
}

/*
 * Test a page replacement algorithm that replaces a page according to
 * the first-in-first-out policy.
 */
TEST(page_replacement, fifo_empty)
{
    auto m = 4u;
    auto A = page_replacement::FIFO(m);
    auto w = page_replacement::PageReferenceString{};
    ASSERT_EQ(0u, page_replacement::cost(A, w));
}

TEST(page_replacement, fifo_single_page_single_reference)
{
    auto m = 4u;
    auto A = page_replacement::FIFO(m);
    auto w = page_replacement::PageReferenceString{0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, fifo_single_page_multiple_references)
{
    auto m = 4u;
    auto A = page_replacement::FIFO(m);
    auto w = page_replacement::PageReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, fifo_replacement)
{
    auto m = 4u;
    auto A = page_replacement::FIFO(m);
    auto w = page_replacement::PageReferenceString{0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u, 0u};
    ASSERT_EQ(6u, page_replacement::cost(A, w));
}

TEST(page_replacement, fifo_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = page_replacement::FIFO(m, std::vector<page_replacement::Page>{0u, 1u, 2u});
    auto w = page_replacement::PageReferenceString{0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

/*
 * Test a page replacement algorithm that replaces a page according to
 * the least recently used policy.
 */
TEST(page_replacement, lru_empty)
{
    auto m = 4u;
    auto A = page_replacement::LRU(m);
    auto w = page_replacement::PageReferenceString{};
    ASSERT_EQ(0u, page_replacement::cost(A, w));
}

TEST(page_replacement, lru_single_page_single_reference)
{
    auto m = 4u;
    auto A = page_replacement::LRU(m);
    auto w = page_replacement::PageReferenceString{0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, lru_single_page_multiple_references)
{
    auto m = 4u;
    auto A = page_replacement::LRU(m);
    auto w = page_replacement::PageReferenceString{0u, 0u, 0u, 0u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

TEST(page_replacement, lru_replacement)
{
    auto m = 4u;
    auto A = page_replacement::LRU(m);
    auto w = page_replacement::PageReferenceString{0u, 1u, 0u, 2u, 0u, 3u, 0u, 4u, 0u};
    ASSERT_EQ(5u, page_replacement::cost(A, w));
}

TEST(page_replacement, lru_replacement_with_initial_state)
{
    auto m = 4u;
    auto A = page_replacement::LRU(m, std::vector<page_replacement::Page>{0u, 1u, 2u});
    auto w = page_replacement::PageReferenceString{0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u};
    ASSERT_EQ(1u, page_replacement::cost(A, w));
}

/*
 * Test counting the number of page replacements for multiple threads
 * with a shared cache.
 */
TEST(page_replcament, lru_replacement_two_threads_shared_cache)
{
    {
        auto m = 4u;
        auto A = page_replacement::LRU(m, std::vector<page_replacement::Page>{0u, 1u, 2u});
        auto ws = std::vector<page_replacement::PageReferenceString>{
            {0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u},
            {}};
        auto cost = page_replacement::cost(A, ws);
        ASSERT_EQ(1u, cost[0]);
        ASSERT_EQ(0u, cost[1]);
    }

    {
        auto m = 4u;
        auto A = page_replacement::LRU(m, std::vector<page_replacement::Page>{0u, 1u, 2u});
        auto ws = std::vector<page_replacement::PageReferenceString>{
            {0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u},
            {0u, 1u, 2u, 3u}};
        auto cost = page_replacement::cost(A, ws);
        ASSERT_EQ(1u, cost[0]);
        ASSERT_EQ(0u, cost[1]);
    }

    {
        auto m = 4u;
        auto A = page_replacement::LRU(m, std::vector<page_replacement::Page>{0u, 1u, 2u});
        auto ws = std::vector<page_replacement::PageReferenceString>{
            {0u, 1u, 2u, 3u, 2u, 7u, 2u, 3u},
            {4u, 5u, 6u, 7u, 6u, 5u, 6u, 7u}};
        auto cost = page_replacement::cost(A, ws);
        ASSERT_EQ(3u, cost[0]);
        ASSERT_EQ(6u, cost[1]);
    }
}
