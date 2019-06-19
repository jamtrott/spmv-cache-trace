#include "cache-simulation/page-replacement.hpp"

#include <iterator>
#include <numeric>
#include <ostream>
#include <utility>

namespace page_replacement
{

unsigned int cost(
    PagingAlgorithm & A,
    PageReferenceString const & w)
{
    auto cost = 0ul;
    for (auto const & x : w)
        cost = cost + A.allocate(x);
    return cost;
}

std::vector<unsigned int> cost(
    PagingAlgorithm & A,
    std::vector<PageReferenceString> const & ws)
{
    auto P = ws.size();

    // Get the the length of each CPU's reference string and the
    // longest reference string.
    std::vector<unsigned int> T(P, 0u);
    auto T_max = 0u;
    for (auto p = 0u; p < P; ++p) {
        T[p] = ws[p].size();
        if (T_max < T[p])
            T_max = T[p];
    }

    // Compute the number of page placements for an interleaved
    // reference string.
    std::vector<unsigned int> cost(P, 0u);
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
    PageReferenceString const & w)
{
    if (w.size() == 0u)
        return o << "()";

    o << '(';
    std::copy(std::begin(w), std::end(w) - 1u,
              std::ostream_iterator<page_replacement::Page>(o, ", "));
    return o << w[w.size()-1u] << ')';
}

}
