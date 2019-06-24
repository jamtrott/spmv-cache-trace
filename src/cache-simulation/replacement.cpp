#include "cache-simulation/replacement.hpp"

#include <iterator>
#include <numeric>
#include <ostream>
#include <utility>

namespace replacement
{

std::vector<cache_miss_type> cost(
    ReplacementAlgorithm & A,
    MemoryReferenceString const & w,
    numa_domain_type num_numa_domains)
{
    std::vector<cache_miss_type> cache_misses(num_numa_domains, 0);
    for (auto const & x : w) {
        memory_reference_type const & memory_reference = x.first;
        numa_domain_type const & numa_domain = x.second;
        cache_misses[numa_domain] += A.allocate(memory_reference, numa_domain);
    }
    return cache_misses;
}

std::vector<std::vector<cache_miss_type>> cost(
    ReplacementAlgorithm & A,
    std::vector<MemoryReferenceString> const & ws,
    numa_domain_type num_numa_domains)
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
    std::vector<std::vector<cache_miss_type>> cache_misses(
        P, std::vector<cache_miss_type>(num_numa_domains, 0));

    for (auto t = 0u; t < T_max; ++t) {
        for (auto p = 0u; p < P; ++p) {
            if (t < T[p]) {
                memory_reference_type const & memory_reference = ws[p][t].first;
                numa_domain_type const & numa_domain = ws[p][t].second;
                cache_misses[p][numa_domain] += A.allocate(memory_reference, numa_domain);
            }
        }
    }
    return cache_misses;
}

std::ostream & operator<<(
    std::ostream & o,
    std::pair<memory_reference_type, numa_domain_type> const & x)
{
    return o << '(' << x.first << ',' << x.second << ')';
}

std::ostream & operator<<(
    std::ostream & o,
    MemoryReferenceString const & w)
{
    if (w.size() == 0u)
        return o << "()";

    o << '(';
    for (size_t i = 0; i < w.size()-1; i++)
        o << w[i] << ", ";
    return o << w[w.size()-1] << ')';
}

}
