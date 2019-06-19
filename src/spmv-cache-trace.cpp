#include "spmv-cache-trace.hpp"

#include "cache-simulation/replacement.hpp"

#include <iterator>
#include <numeric>
#include <ostream>

CacheComplexityEstimate estimate_spmv_cache_complexity_private(
    linsparse::Matrix const & matrix,
    linsparse::Vector const & x,
    linsparse::Vector const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_size,
    unsigned int cache_line_size)
{
    auto cache_lines = (cache_size + cache_line_size - 1u) / cache_line_size;
    auto A = replacement::LRU(cache_lines);
    auto w = matrix.spmv_memory_reference_reference_string(
        x, y, thread, num_threads, cache_line_size);
    auto cache_misses = replacement::cost(A, w);
    return cache_misses;
}

std::vector<CacheComplexityEstimate> estimate_spmv_cache_complexity_private(
    linsparse::Matrix const & matrix,
    linsparse::Vector const & x,
    linsparse::Vector const & y,
    unsigned int num_threads,
    unsigned int cache_size,
    unsigned int cache_line_size)
{
    std::vector<CacheComplexityEstimate> cache_complexity(
        num_threads, CacheComplexityEstimate{});
    for (auto t = 0u; t < num_threads; t++) {
        cache_complexity[t] = estimate_spmv_cache_complexity_private(
            matrix, x, y, t, num_threads, cache_size, cache_line_size);
    }
    return cache_complexity;
}

std::vector<CacheComplexityEstimate> estimate_spmv_cache_complexity_shared(
    linsparse::Matrix const & matrix,
    linsparse::Vector const & x,
    linsparse::Vector const & y,
    unsigned int num_threads,
    unsigned int cache_size,
    unsigned int cache_line_size)
{
    using RefString = replacement::MemoryReferenceString;
    std::vector<RefString> ws(num_threads);
    std::vector<RefString::const_iterator> its(num_threads);
    std::vector<RefString::const_iterator> ends(num_threads);
    for (auto n = 0u; n < num_threads; n++) {
        ws[n] = matrix.spmv_memory_reference_reference_string(
            x, y, n, num_threads, cache_line_size);
        its[n] = std::cbegin(ws[n]);
        ends[n] = std::cend(ws[n]);
    }

    auto cache_lines = (cache_size + cache_line_size - 1u) / cache_line_size;
    auto A = replacement::LRU(cache_lines);
    auto cache_misses = replacement::cost(A, ws);
    return cache_misses;
}

std::vector<CacheComplexityEstimate> estimate_spmv_cache_complexity(
    linsparse::Matrix const & matrix,
    linsparse::Vector const & x,
    linsparse::Vector const & y,
    unsigned int num_threads,
    unsigned int cache_size,
    unsigned int cache_line_size,
    bool shared_cache)
{
    if (shared_cache && num_threads > 1u) {
        return estimate_spmv_cache_complexity_shared(
            matrix, x, y, num_threads, cache_size, cache_line_size);
    } else {
        return estimate_spmv_cache_complexity_private(
            matrix, x, y, num_threads, cache_size, cache_line_size);
    }
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<CacheComplexityEstimate> const & v)
{
    if (v.size() == 0u)
        return o << "[]";

    o << "[\n";
    for (auto i = 0ul; i < v.size()-1ul; i++)
        o << "    " << v[i] << ",\n";
    return o << "    " << v[v.size()-1u] << "\n  ]";
}
