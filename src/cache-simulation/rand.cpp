#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <set>

namespace replacement
{

using cache_reference_type = uintptr_t;

RAND::RAND(
    cache_size_type cache_lines,
    cache_size_type cache_line_size,
    MemoryReferenceSet const & memory_references)
    : ReplacementAlgorithm(
        cache_lines,
        cache_line_size,
        memory_references)
{
}

RAND::~RAND()
{
}

cache_miss_type RAND::allocate(
    memory_reference_type x,
    numa_domain_type numa_domain)
{
    cache_reference_type y = x / cache_line_size;
    auto it = std::find(std::cbegin(memory_references),
                        std::cend(memory_references), y);
    if (it != std::cend(memory_references))
        return 0u;
    if (memory_references.size() == cache_lines)
        memory_references.erase(std::begin(memory_references));
    memory_references.insert(y);
    return 1u;
}

}
