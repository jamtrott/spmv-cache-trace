#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <numeric>
#include <ostream>
#include <vector>

template <typename T, class Allocator>
std::vector<T, Allocator> operator*(
    std::vector<T, Allocator> const & x,
    T const & a)
{
    std::vector<T, Allocator> y(x.size(), 0.0);
    std::transform(
        std::begin(x), std::end(x), std::begin(x), std::begin(y),
        [&a] (auto const & r, auto const &) { return r * a; });
    return y;
}

template <typename T, class Allocator>
std::vector<T, Allocator> operator*(
    T const & a,
    std::vector<T, Allocator> const & x)
{
    std::vector<T, Allocator> y(x.size(), 0.0);
    std::transform(
        std::begin(x), std::end(x), std::begin(x), std::begin(y),
        [&a] (auto const & r, auto const &) { return r * a; });
    return y;
}

template <typename T, class Allocator>
std::vector<T, Allocator> operator+(
    std::vector<T, Allocator> const & a,
    std::vector<T, Allocator> const & b)
{
    std::vector<T, Allocator> c(a.size(), 0.0);
    std::transform(
        std::begin(a), std::end(a), std::begin(b), std::begin(c),
        std::plus<T>());
    return c;
}

template <typename T, class Allocator>
std::vector<T, Allocator> operator-(
    std::vector<T, Allocator> const & a,
    std::vector<T, Allocator> const & b)
{
    std::vector<T, Allocator> c(a.size(), 0.0);
    std::transform(
        std::begin(a), std::end(a), std::begin(b), std::begin(c),
        std::minus<T>());
    return c;
}

template <typename T, class Allocator>
T inner(
    std::vector<T, Allocator> const & a,
    std::vector<T, Allocator> const & b)
{
    return std::inner_product(
        std::begin(a), std::end(a), std::begin(b), 0.0);
}

template <typename T, class Allocator>
T l2norm(std::vector<T, Allocator> const & x)
{
    return std::sqrt(inner(x, x));
}

template <typename T, class Allocator>
std::ostream & operator<<(
    std::ostream & o, std::vector<T, Allocator> const & v)
{
    if (v.size() == 0u)
        return o << "()";

    o << '(';
    std::copy(std::begin(v), std::end(v) - 1u,
              std::ostream_iterator<T>(o, ", "));
    return o << v[v.size()-1u] << ')';
}

#endif
