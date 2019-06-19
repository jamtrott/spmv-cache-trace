#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>

namespace replacement
{

FIFO::FIFO(
    unsigned int cache_lines,
    std::vector<Page> const & initial_state)
    : PagingAlgorithm(
        cache_lines,
        MemoryReferenceSet(std::begin(initial_state), std::end(initial_state)))
    , q()
{
    for (auto & page : initial_state)
        q.emplace(page);
}

FIFO::~FIFO()
{
}

AllocationCost FIFO::allocate(Page const & x)
{
    auto it = pages.find(x);
    if (it != std::cend(pages) && x == *it)
        return 0u;

    pages.insert(x);
    if (pages.size() > cache_lines) {
        auto y = q.front();
        q.pop();
        pages.erase(y);
    }
    q.emplace(x);
    return 1u;
}

}
