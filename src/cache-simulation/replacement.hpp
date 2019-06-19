#ifndef REPLACEMENT_HPP
#define REPLACEMENT_HPP

/*
 * Performance models for page replacement algorithms, loosely
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
constexpr MemoryReference undefined_page = std::numeric_limits<MemoryReference>::max();

using AllocationCost = unsigned int;

/*
 * Paging algorithms.
 */
class PagingAlgorithm
{
public:
    PagingAlgorithm(
        unsigned int cache_lines,
        MemoryReferenceSet const & _pages)
        : cache_lines(cache_lines)
        , pages(_pages)
    {
        pages.reserve(cache_lines);
    }

    virtual ~PagingAlgorithm()
    {
    }

    virtual AllocationCost allocate(MemoryReference const & x) = 0;

protected:
    // The number of cache lines that fit in the cache
    unsigned int cache_lines;

    // The subset cache lines residing in the cache
    MemoryReferenceSet pages;
};

/*
 * A random page replacement policy.
 */
class RAND
    : public PagingAlgorithm
{
public:
    RAND(
        unsigned int cache_lines,
        MemoryReferenceSet const & pages = MemoryReferenceSet());
    ~RAND();

    AllocationCost allocate(MemoryReference const & x) override;
};

/*
 * A first-in-first-out page replacement policy.
 */
class FIFO
    : public PagingAlgorithm
{
public:
    FIFO(
        unsigned int cache_lines,
        std::vector<MemoryReference> const & pages = std::vector<MemoryReference>());
    ~FIFO();

    AllocationCost allocate(MemoryReference const & x) override;

private:
    std::queue<MemoryReference> q;
};

/*
 * A least-recently used page replacement policy.
 */
class LRU
    : public PagingAlgorithm
{
public:
    LRU(
        unsigned int cache_lines,
        std::vector<MemoryReference> const & pages = std::vector<MemoryReference>());
    ~LRU();

    AllocationCost allocate(MemoryReference const & x) override;

private:
    CircularBuffer<MemoryReference> q;
};

/*
 * Compute the cost (number of page placements) of processing a
 * page reference string with a given paging algorithm and initial state.
 */
unsigned int cost(
    PagingAlgorithm & A,
    MemoryReferenceString const & w);

/*
 * Compute the cost (number of page placements) of processing page
 * reference strings for multiple processors with a shared cache.
 *
 * In this case, it is assumed that the page reference string of the
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
