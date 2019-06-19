#ifndef SPMV_CACHE_TRACE_HPP
#define SPMV_CACHE_TRACE_HPP

#include "matrix/matrix.hpp"

#include <iosfwd>

using CacheComplexityEstimate = unsigned int;

std::vector<CacheComplexityEstimate> estimate_spmv_cache_complexity(
    linsparse::Matrix const & matrix,
    linsparse::Vector const & x,
    linsparse::Vector const & y,
    unsigned int num_threads,
    unsigned int cache_size,
    unsigned int cache_line_size,
    bool shared_cache);

std::ostream & operator<<(
    std::ostream & o,
    std::vector<CacheComplexityEstimate> const & v);

#endif
