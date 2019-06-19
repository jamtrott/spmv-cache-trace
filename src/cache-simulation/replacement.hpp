#ifndef REPLACEMENT_HPP
#define REPLACEMENT_HPP

/*
 * Performance models for replacement algorithms, loosely
 * based on the paper:
 *
 * Alfred V. Aho, Peter J. Denning, and Jeffrey D. Ullman (1971):
 * Principles of Optimal Page Replacement, in J. ACM, vol. 18, no. 1,
 * pp. 80--93. DOI=http://dx.doi.org/10.1145/321623.321632.
 */

#include "util/circular-buffer.hpp"

#include <functional>
#include <iosfwd>
#include <queue>
#include <unordered_set>
#include <vector>

namespace replacement
{

using MemoryReference = uintptr_t;
using MemoryReferenceString = std::vector<MemoryReference>;
using MemoryReferenceSet = std::unordered_set<MemoryReference>;
constexpr MemoryReference undefined_memory_reference = std::numeric_limits<MemoryReference>::max();

using AllocationCost = unsigned int;

/*
 * Paging algorithms.
 */
class PagingAlgorithm
{
public:
    PagingAlgorithm(
        unsigned int cache_lines,
        MemoryReferenceSet const & _memory_references)
        : cache_lines(cache_lines)
        , memory_references(_memory_references)
    {
        memory_references.reserve(cache_lines);
    }

    virtual ~PagingAlgorithm()
    {
    }

    virtual AllocationCost allocate(MemoryReference const & x) = 0;

protected:
    // The number of cache lines that fit in the cache
    unsigned int cache_lines;

    // The subset cache lines residing in the cache
    MemoryReferenceSet memory_references;
};

/*
 * A random replacement policy.
 */
class RAND
    : public PagingAlgorithm
{
public:
    RAND(
        unsigned int cache_lines,
        MemoryReferenceSet const & memory_references = MemoryReferenceSet());
    ~RAND();

    AllocationCost allocate(MemoryReference const & x) override;
};

/*
 * A first-in-first-out replacement policy.
 */
class FIFO
    : public PagingAlgorithm
{
public:
    FIFO(
        unsigned int cache_lines,
        std::vector<MemoryReference> const & memory_references = std::vector<MemoryReference>());
    ~FIFO();

    AllocationCost allocate(MemoryReference const & x) override;

private:
    std::queue<MemoryReference> q;
};

/*
 * A least-recently used replacement policy.
 */
class LRU
    : public PagingAlgorithm
{
public:
    LRU(
        unsigned int cache_lines,
        std::vector<MemoryReference> const & memory_references = std::vector<MemoryReference>());
    ~LRU();

    AllocationCost allocate(MemoryReference const & x) override;

private:
    CircularBuffer<MemoryReference> q;
};

/*
 * Compute the cost (number of replacements) of processing a memory
 * reference string with a given replacement algorithm and initial state.
 */
unsigned int cost(
    PagingAlgorithm & A,
    MemoryReferenceString const & w);

/*
 * Compute the cost (number of replacements) of processing memory
 * reference strings for multiple processors with a shared cache.
 *
 * In this case, it is assumed that the memory reference strings of the
 * different CPUs are perfectly interleaved.  In reality, scheduling
 * may be unfair and memory access latencies vary, causing some CPUs
 * to be delayed more than others.
 */
std::vector<unsigned int> cost(
    PagingAlgorithm & A,
    std::vector<MemoryReferenceString> const & ws);

std::ostream & operator<<(
    std::ostream & o,
    MemoryReferenceString const & v);

}

#endif
