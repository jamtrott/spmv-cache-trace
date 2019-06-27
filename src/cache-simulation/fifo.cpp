#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>

namespace replacement
{

using cache_reference_type = uintptr_t;

FIFO::FIFO(
    cache_size_type cache_lines,
    cache_size_type cache_line_size,
    std::vector<memory_reference_type> const & initial_state)
    : ReplacementAlgorithm(
        cache_lines,
        cache_line_size,
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
    cache_reference_type y = x / cache_line_size;
    auto it = memory_references.find(y);
    if (it != std::cend(memory_references) && y == *it)
        return 0u;

    memory_references.insert(y);
    if (memory_references.size() > cache_lines) {
        auto z = q.front();
        q.pop();
        memory_references.erase(z);
    }
    q.emplace(y);
    return 1u;
}

}
