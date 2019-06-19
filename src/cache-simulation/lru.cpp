#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <vector>

namespace replacement
{

LRU::LRU(
    unsigned int cache_lines,
    std::vector<Page> const & initial_state)
    : PagingAlgorithm(
        cache_lines,
        PageSet(std::begin(initial_state), std::end(initial_state)))
    , q(2*cache_lines)
{
    for (auto & page : initial_state)
        q.emplace_back(page);
}

LRU::~LRU()
{
}

AllocationCost LRU::allocate(Page const & x)
{
    auto it = pages.find(x);
    if (it != std::end(pages) && x == *it) {
        auto rit = std::find(std::crbegin(q), std::crend(q), x);
        if (rit != std::crend(q)) {
            auto it = std::begin(q) + q.size() - (rit + 1 - std::crbegin(q));
            std::rotate(it, it + 1, std::end(q));
        }
        return 0u;
    }

    pages.insert(x);
    if (pages.size() > cache_lines) {
        auto y = q.front();
        q.pop_front();
        pages.erase(y);
    }
    q.emplace_back(x);
    return 1u;
}

}
