#include "util/aligned-allocator.hpp"

#include <gtest/gtest.h>

#include <vector>

TEST(aligned_allocator, aligned_vector)
{
    std::vector<int, aligned_allocator<int, 64>> v{1u, 2u, 3u, 4u};
    ASSERT_EQ(4u, v.size());
    ASSERT_EQ(1u, v[0u]);
    ASSERT_EQ(2u, v[1u]);
    ASSERT_EQ(3u, v[2u]);
    ASSERT_EQ(4u, v[3u]);
    ASSERT_EQ(0u, intptr_t(v.data()) % 64);
}
