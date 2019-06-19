#include "util/circular-buffer.hpp"

#include <gtest/gtest.h>

TEST(circular_buffer, empty)
{
    CircularBuffer<int> b(10u);
    ASSERT_TRUE(b.empty());
}

TEST(circular_buffer, push_back)
{
    CircularBuffer<int> b(10u);
    b.push_back(1);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 1);
    ASSERT_EQ(b.front(), 1);
    ASSERT_EQ(b.back(), 1);
}

TEST(circular_buffer, emplace_back)
{
    CircularBuffer<int> b(10u);
    b.emplace_back(1);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 1);
    ASSERT_EQ(b.front(), 1);
    ASSERT_EQ(b.back(), 1);
}

TEST(circular_buffer, pop_front)
{
    CircularBuffer<int> b(10u);
    b.push_back(1);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 1);
    ASSERT_EQ(b.front(), 1);
    ASSERT_EQ(b.back(), 1);
    b.pop_front();
    ASSERT_TRUE(b.empty());
    ASSERT_EQ(b.size(), 0);
}

TEST(circular_buffer, push_back_full)
{
    CircularBuffer<int> b(2u);
    b.push_back(0);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 1);
    ASSERT_EQ(b.front(), 0);
    ASSERT_EQ(b.back(), 0);

    b.push_back(1);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 2);
    ASSERT_EQ(b.front(), 0);
    ASSERT_EQ(b.back(), 1);

    b.push_back(2);
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(b.size(), 2);
    ASSERT_EQ(b.front(), 1);
    ASSERT_EQ(b.back(), 2);
}

TEST(circular_buffer, push_pop_full)
{
    {
        CircularBuffer<int> b(3u);
        b.push_back(0);
        b.push_back(1);
        b.push_back(2);
        b.pop_front();
        ASSERT_FALSE(b.empty());
        ASSERT_EQ(b.size(), 2);
        ASSERT_EQ(b.front(), 1);
        ASSERT_EQ(b.back(), 2);

        b.push_back(3);
        ASSERT_FALSE(b.empty());
        ASSERT_EQ(b.size(), 3);
        ASSERT_EQ(b.front(), 1);
        ASSERT_EQ(b.back(), 3);
    }
    {
        CircularBuffer<int> b(3u);
        b.push_back(0);
        b.push_back(1);
        b.push_back(2);
        b.pop_front();
        b.pop_front();
        b.pop_front();
        ASSERT_TRUE(b.empty());
        ASSERT_EQ(b.size(), 0);

        b.push_back(3);
        ASSERT_FALSE(b.empty());
        ASSERT_EQ(b.size(), 1);
        ASSERT_EQ(b.front(), 3);
        ASSERT_EQ(b.back(), 3);
    }
}

TEST(circular_buffer, iterator)
{
    CircularBuffer<int> b(2u);
    ASSERT_EQ(std::cbegin(b), std::cend(b));

    b.push_back(0);
    ASSERT_EQ(0, *std::next(std::cbegin(b)));
    ASSERT_EQ(std::cend(b), std::next(std::cbegin(b)));

    b.push_back(1);
    ASSERT_EQ(0, *(std::begin(b)));
    ASSERT_EQ(1, *(std::begin(b) + 1u));
    ASSERT_EQ(std::cend(b), std::cbegin(b) + 2u);

    b.push_back(2);
    ASSERT_EQ(1, *(std::begin(b)));
    ASSERT_EQ(2, *(std::begin(b) + 1u));
    ASSERT_EQ(std::cend(b), std::cbegin(b) + 2u);
}

TEST(circular_buffer, reverse_iterator)
{
    CircularBuffer<int> b(2u);
    ASSERT_EQ(std::crbegin(b), std::crend(b));

    b.push_back(0);
    ASSERT_EQ(0, *std::next(std::crbegin(b)));
    ASSERT_EQ(std::crend(b), std::next(std::crbegin(b)));

    b.push_back(1);
    ASSERT_EQ(1, *(std::crbegin(b)));
    ASSERT_EQ(0, *(std::crbegin(b) + 1u));
    ASSERT_EQ(std::crend(b), std::crbegin(b) + 2u);

    b.push_back(2);
    ASSERT_EQ(2, *(std::crbegin(b)));
    ASSERT_EQ(1, *(std::crbegin(b) + 1u));
    ASSERT_EQ(std::crend(b), std::crbegin(b) + 2u);
}
