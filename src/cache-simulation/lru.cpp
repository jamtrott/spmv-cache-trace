#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <vector>

namespace replacement
{

LRU::LRU(
    unsigned int cache_lines,
    std::vector<MemoryReference> const & initial_state)
    : ReplacementAlgorithm(
        cache_lines,
        MemoryReferenceSet(std::begin(initial_state), std::end(initial_state)))
    , q(2*cache_lines)
{
    for (auto & memory_reference : initial_state)
        q.emplace_back(memory_reference);
}

LRU::~LRU()
{
}

AllocationCost LRU::allocate(MemoryReference const & x)
{
    auto it = memory_references.find(x);
    if (it != std::end(memory_references) && x == *it) {
        auto rit = std::find(std::crbegin(q), std::crend(q), x);
        if (rit != std::crend(q)) {
            auto it = std::begin(q) + q.size() - (rit + 1 - std::crbegin(q));
            std::rotate(it, it + 1, std::end(q));
        }
        return 0u;
    }

    memory_references.insert(x);
    if (memory_references.size() > cache_lines) {
        auto y = q.front();
        q.pop_front();
        memory_references.erase(y);
    }
    q.emplace_back(x);
    return 1u;
}

}
