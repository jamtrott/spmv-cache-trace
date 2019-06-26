#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <vector>

template <typename T>
T sample_min(std::vector<T> const & v)
{
    size_t n = v.size();
    T min = std::numeric_limits<T>::max();
    for (size_t i = 0; i < n; i++)
        min = std::min(min, v[i]);
    return min;
}

template <typename T>
T sample_max(std::vector<T> const & v)
{
    size_t n = v.size();
    T max = std::numeric_limits<T>::min();
    for (size_t i = 0; i < n; i++)
        max = std::max(max, v[i]);
    return max;
}

template <typename T>
double sample_mean(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    size_t n = v.size();
    double mu = 0.0;
    for (size_t i = 0; i < n; i++)
        mu = mu + v[i];
    return mu / (double) n;
}

template <typename T>
double sample_median(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    size_t n = v.size();
    std::vector<T> v_sorted(v);
    std::sort(std::begin(v_sorted), std::end(v_sorted));
    if (n % 1 == 0)
        return v_sorted[n/2];
    else return 0.5 * (v_sorted[(n-1)/2] + v_sorted[n/2]);
}

template <typename T>
double sample_second_moment(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double mu = sample_mean(v);
    size_t n = v.size();
    double m = 0.0;
    for (size_t i = 0; i < n; i++)
        m = m + (v[i] - mu) * (v[i] - mu);
    return m / (double) n;
}

template <typename T>
double sample_third_moment(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double mu = sample_mean(v);
    size_t n = v.size();
    double m = 0.0;
    for (size_t i = 0; i < n; i++)
        m = m + (v[i] - mu) * (v[i] - mu) * (v[i] - mu);
    return m / (double) n;
}

template <typename T>
double sample_fourth_moment(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double mu = sample_mean(v);
    size_t n = v.size();
    double m = 0.0;
    for (size_t i = 0; i < n; i++)
        m = m + (v[i] - mu) * (v[i] - mu) * (v[i] - mu) * (v[i] - mu);
    return m / (double) n;
}

template <typename T>
double sample_variance(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double mu = sample_mean(v);
    size_t n = v.size();
    double s = 0.0;
    for (size_t i = 0; i < n; i++)
        s = s + (v[i] - mu) * (v[i] - mu);
    return s / (double) (n-1);
}

template <typename T>
double sample_standard_deviation(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double s = sample_variance(v);
    return std::sqrt(s);
}

template <typename T>
double sample_skewness(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double s = sample_variance(v);
    double m = sample_third_moment(v);
    return m / std::sqrt(s*s*s);
}

template <typename T>
double sample_kurtosis(std::vector<T> const & v)
{
    if (v.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    double m2 = sample_second_moment(v);
    double m4 = sample_fourth_moment(v);
    return m4 / (m2*m2);
}

template <typename T>
std::ostream & print_sample(
    std::ostream & o,
    std::vector<T> const & v,
    std::string const & unit)
{
    return o << '{' << '\n'
             << '"' << "samples" << '"' << ": "
             << v.size() << ',' << '\n'
             << '"' << "min" << '"' << ": "
             << sample_min(v) << ',' << '\n'
             << '"' << "max" << '"' << ": "
             << sample_max(v) << ',' << '\n'
             << '"' << "mean" << '"' << ": "
             << sample_mean(v) << ',' << '\n'
             << '"' << "median" << '"' << ": "
             << sample_median(v) << ',' << '\n'
             << '"' << "variance" << '"' << ": "
             << sample_variance(v) << ',' << '\n'
             << '"' << "standard_deviation" << '"' << ": "
             << sample_standard_deviation(v) << ',' << '\n'
             << '"' << "skewness" << '"' << ": "
             << sample_skewness(v) << ',' << '\n'
             << '"' << "kurtosis" << '"' << ": "
             << sample_kurtosis(v) << ',' << '\n'
             << '"' << "unit" << '"' << ": "
             << '"' << unit << '"'
             << '\n' << '}';
}

#endif
