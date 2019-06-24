#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>

namespace replacement
{

FIFO::FIFO(
    cache_size_type cache_lines,
    std::vector<memory_reference_type> const & initial_state)
    : ReplacementAlgorithm(
        cache_lines,
        MemoryReferenceSet(std::begin(initial_state), std::end(initial_state)))
    , q()
{
    for (auto & memory_reference : initial_state)
        q.emplace(memory_reference);
}

FIFO::~FIFO()
{
}

cache_miss_type FIFO::allocate(
    memory_reference_type x,
    numa_domain_type numa_domain)
{
    auto it = memory_references.find(x);
    if (it != std::cend(memory_references) && x == *it)
        return 0u;

    memory_references.insert(x);
    if (memory_references.size() > cache_lines) {
        auto y = q.front();
        q.pop();
        memory_references.erase(y);
    }
    q.emplace(x);
    return 1u;
}

}
