#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>

namespace replacement
{

FIFO::FIFO(
    unsigned int cache_lines,
    std::vector<MemoryReference> const & initial_state)
    : PagingAlgorithm(
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

AllocationCost FIFO::allocate(MemoryReference const & x)
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
