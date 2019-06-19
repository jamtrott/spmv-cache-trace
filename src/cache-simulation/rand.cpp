#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <set>

namespace replacement
{

RAND::RAND(
    unsigned int cache_lines,
    MemoryReferenceSet const & pages)
    : PagingAlgorithm(cache_lines, pages)
{
}

RAND::~RAND()
{
}

AllocationCost RAND::allocate(MemoryReference const & x)
{
    if (std::find(std::cbegin(pages), std::cend(pages), x) != std::cend(pages)) {
        return 0u;
    }

    if (pages.size() == cache_lines) {
        auto y = *std::begin(pages);
        pages.erase(std::begin(pages));
    }
    pages.insert(x);
    return 1u;
}

}
