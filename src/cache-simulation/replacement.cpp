#include "cache-simulation/replacement.hpp"

#include <iterator>
#include <numeric>
#include <iostream>
#include <ostream>
#include <utility>

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

namespace replacement
{

std::vector<cache_miss_type> trace_cache_misses(
    ReplacementAlgorithm & A,
    MemoryReferenceString const & w,
    numa_domain_type num_numa_domains,
    bool verbose)
{
    std::vector<cache_miss_type> cache_misses(num_numa_domains, 0);
    for (auto const & x : w) {
        memory_reference_type const & memory_reference = x.first;
        numa_domain_type const & numa_domain = x.second;
        cache_misses[numa_domain] +=
            A.allocate(memory_reference, numa_domain);
    }
    return cache_misses;
}

volatile sig_atomic_t print_progress = 0;

void signal_handler(int status)
{
    print_progress = 1;
}

std::vector<std::vector<cache_miss_type>> trace_cache_misses(
    ReplacementAlgorithm & A,
    std::vector<MemoryReferenceString> const & ws,
    numa_domain_type num_numa_domains,
    bool verbose,
    int progress_interval)
{
    auto P = ws.size();

    // Get the the length of each CPU's reference string and the
    // longest reference string.
    std::vector<memory_reference_type> T(P, 0u);
    uint64_t T_max = 0;
    for (auto p = 0u; p < P; ++p) {
        T[p] = ws[p].size();
        if (T_max < T[p])
            T_max = T[p];
    }

    // Compute the number of replacements for an interleaved
    // reference string.
    std::vector<std::vector<cache_miss_type>> cache_misses(
        P, std::vector<cache_miss_type>(num_numa_domains, 0));

    if (verbose && progress_interval > 0) {
        print_progress = 0;
        signal(SIGALRM, signal_handler);
        alarm(progress_interval);
    }

    for (uint64_t t = 0; t < T_max; ++t) {
        if (verbose && progress_interval > 0 && print_progress) {
            fprintf(stderr, "%'" PRIu64 " of %'" PRIu64 " (%4.1f %%)\n",
                    t, T_max, T_max > 0 ? 100.0 * (t / (double) T_max) : 0.0);
            print_progress = 0;
            alarm(progress_interval);
        }

        for (auto p = 0u; p < P; ++p) {
            if (t < T[p]) {
                memory_reference_type const & memory_reference = ws[p][t].first;
                numa_domain_type const & numa_domain = ws[p][t].second;
                cache_misses[p][numa_domain] +=
                    A.allocate(memory_reference, numa_domain);
            }
        }
    }

    if (verbose && progress_interval > 0) {
        alarm(0);
        signal(SIGALRM, SIG_DFL);
        fprintf(stderr, "%'" PRIu64 " of %'" PRIu64 " (%4.1f %%)\n", T_max, T_max, 100.0);
    }
    return cache_misses;
}

std::ostream & operator<<(
    std::ostream & o,
    std::pair<memory_reference_type, numa_domain_type> const & x)
{
    return o << '(' << x.first << ',' << x.second << ')';
}

std::ostream & operator<<(
    std::ostream & o,
    MemoryReferenceString const & w)
{
    if (w.size() == 0u)
        return o << "()";

    o << '(';
    for (size_t i = 0; i < w.size()-1; i++)
        o << w[i] << ", ";
    return o << w[w.size()-1] << ')';
}

}
