#include "cache-simulation/replacement.hpp"

#include <iterator>
#include <numeric>
#include <ostream>
#include <utility>

namespace replacement
{

cache_miss_type cost(
    ReplacementAlgorithm & A,
    MemoryReferenceString const & w)
{
    auto cost = 0ul;
    for (auto const & x : w)
        cost = cost + A.allocate(x);
    return cost;
}

std::vector<cache_miss_type> cost(
    ReplacementAlgorithm & A,
    std::vector<MemoryReferenceString> const & ws)
{
    auto P = ws.size();

    // Get the the length of each CPU's reference string and the
    // longest reference string.
    std::vector<memory_reference_type> T(P, 0u);
    auto T_max = 0u;
    for (auto p = 0u; p < P; ++p) {
        T[p] = ws[p].size();
        if (T_max < T[p])
            T_max = T[p];
    }

    // Compute the number of replacements for an interleaved
    // reference string.
    std::vector<cache_miss_type> cost(P, 0u);
    for (auto t = 0u; t < T_max; ++t) {
        for (auto p = 0u; p < P; ++p) {
            if (t < T[p]) {
                cost[p] += A.allocate(ws[p][t]);
            }
        }
    }

    return cost;
}

std::ostream & operator<<(
    std::ostream & o,
    MemoryReferenceString const & w)
{
    if (w.size() == 0u)
        return o << "()";

    o << '(';
    std::copy(std::begin(w), std::end(w) - 1u,
              std::ostream_iterator<replacement::memory_reference_type>(o, ", "));
    return o << w[w.size()-1u] << ')';
}

}
