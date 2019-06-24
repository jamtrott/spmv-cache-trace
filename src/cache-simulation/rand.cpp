#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <set>

namespace replacement
{

RAND::RAND(
    cache_size_type cache_lines,
    MemoryReferenceSet const & memory_references)
    : ReplacementAlgorithm(cache_lines, memory_references)
{
}

RAND::~RAND()
{
}

cache_miss_type RAND::allocate(
    memory_reference_type x,
    numa_domain_type numa_domain)
{
    if (std::find(std::cbegin(memory_references), std::cend(memory_references), x) != std::cend(memory_references)) {
        return 0u;
    }

    if (memory_references.size() == cache_lines) {
        memory_references.erase(std::begin(memory_references));
    }
    memory_references.insert(x);
    return 1u;
}

}
