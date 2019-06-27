#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <vector>

namespace replacement
{

using cache_reference_type = uintptr_t;

LRU::LRU(
    cache_size_type cache_lines,
    cache_size_type cache_line_size,
    std::vector<memory_reference_type> const & initial_state)
    : ReplacementAlgorithm(
        cache_lines,
        cache_line_size,
        MemoryReferenceSet(std::begin(initial_state), std::end(initial_state)))
    , q(2*cache_lines)
{
    for (auto & memory_reference : initial_state)
        q.emplace_back(memory_reference);
}

LRU::~LRU()
{
}

cache_miss_type LRU::allocate(
    memory_reference_type x,
    numa_domain_type numa_domain)
{
    cache_reference_type y = x / cache_line_size;
    auto it = memory_references.find(y);
    if (it != std::end(memory_references) && y == *it) {
        auto rit = std::find(std::crbegin(q), std::crend(q), y);
        if (rit != std::crend(q)) {
            auto it = std::begin(q) + q.size() - (rit + 1 - std::crbegin(q));
            std::rotate(it, it + 1, std::end(q));
        }
        return 0u;
    }

    memory_references.insert(y);
    if (memory_references.size() > cache_lines) {
        auto z = q.front();
        q.pop_front();
        memory_references.erase(z);
    }
    q.emplace_back(y);
    return 1u;
}

}
