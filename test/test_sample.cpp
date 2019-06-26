#include <gtest/gtest.h>

#include <vector>

template <typename T>
double sample_mean(std::vector<T> const & v)
{
    if (v.size() == 0)
        throw std::invalid_argument("Expected non-empty sample");
    size_t n = v.size();
    double mu = 0.0;
    for (size_t i = 0; i < n; i++)
        mu = mu + v[i];
    return mu / (double) n;
}

template <typename T>
double sample_variance(std::vector<T> const & v)
{
    if (v.size() == 0)
        throw std::invalid_argument("Expected non-empty sample");
    double mu = sample_mean(v);
    size_t n = v.size();
    double s = 0.0;
    for (size_t i = 0; i < n; i++)
        s = s + (v[i] - mu) * (v[i] - mu);
    return s / (double) (n-1);
}

TEST(sample, mean)
{
    std::vector<int> v{0u, 1u, 2u, 3u, 4u};
    ASSERT_EQ(2.0, sample_mean(v));
    ASSERT_EQ(2.5, sample_variance(v));
}
