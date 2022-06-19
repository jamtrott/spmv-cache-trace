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

using memory_reference_type = uintptr_t;
using numa_domain_type = int;
using cache_size_type = uint64_t;
using cache_miss_type = uint64_t;

using MemoryReferenceString =
    std::vector<std::pair<memory_reference_type, numa_domain_type>>;
using MemoryReferenceSet = std::unordered_set<memory_reference_type>;

/*
 * Replacement algorithms.
 */
class ReplacementAlgorithm
{
public:
    ReplacementAlgorithm(
        cache_size_type cache_lines,
        cache_size_type cache_line_size,
        MemoryReferenceSet const & _memory_references)
        : cache_lines(cache_lines)
        , cache_line_size(cache_line_size)
        , memory_references(_memory_references)
    {
        memory_references.reserve(cache_lines);
    }

    virtual ~ReplacementAlgorithm()
    {
    }

    virtual cache_miss_type allocate(
        memory_reference_type x,
        numa_domain_type numa_domain) = 0;

protected:
    // The number of cache lines that fit in the cache
    cache_size_type cache_lines;

    // The size of each cache line (in bytes)
    cache_size_type cache_line_size;

    // The subset cache lines residing in the cache
    MemoryReferenceSet memory_references;
};

/*
 * A random replacement policy.
 */
class RAND
    : public ReplacementAlgorithm
{
public:
    RAND(
        cache_size_type cache_lines,
        cache_size_type cache_line_size,
        MemoryReferenceSet const & memory_references = MemoryReferenceSet());
    ~RAND();

    cache_miss_type allocate(
        memory_reference_type x,
        numa_domain_type numa_domain) override;
};

/*
 * A first-in-first-out replacement policy.
 */
class FIFO
    : public ReplacementAlgorithm
{
public:
    FIFO(
        cache_size_type cache_lines,
        cache_size_type cache_line_size,
        std::vector<memory_reference_type> const & memory_references = std::vector<memory_reference_type>());
    ~FIFO();

    cache_miss_type allocate(
        memory_reference_type x,
        numa_domain_type numa_domain) override;

private:
    std::queue<memory_reference_type> q;
};

/*
 * A least-recently used replacement policy.
 */
class LRU
    : public ReplacementAlgorithm
{
public:
    LRU(
        cache_size_type cache_lines,
        cache_size_type cache_line_size,
        std::vector<memory_reference_type> const & memory_references = std::vector<memory_reference_type>());
    ~LRU();

    cache_miss_type allocate(
        memory_reference_type x,
        numa_domain_type numa_domain) override;

private:
    CircularBuffer<memory_reference_type> q;
};

/*
 * Compute the cost (number of replacements) of processing a memory
 * reference string with a given replacement algorithm and initial state.
 */
std::vector<cache_miss_type> trace_cache_misses(
    ReplacementAlgorithm & A,
    MemoryReferenceString const & w,
    numa_domain_type num_numa_domains,
    bool verbose,
    int progress_interval);

/*
 * Compute the cost (number of replacements) of processing memory
 * reference strings for multiple processors with a shared cache.
 *
 * In this case, it is assumed that the memory reference strings of the
 * different CPUs are perfectly interleaved.  In reality, scheduling
 * may be unfair and memory access latencies vary, causing some CPUs
 * to be delayed more than others.
 */
std::vector<std::vector<cache_miss_type>> trace_cache_misses(
    ReplacementAlgorithm & A,
    std::vector<MemoryReferenceString> const & ws,
    numa_domain_type num_numa_domains,
    bool verbose,
    int progress_interval);

std::ostream & operator<<(
    std::ostream & o,
    MemoryReferenceString const & v);

}

#endif
