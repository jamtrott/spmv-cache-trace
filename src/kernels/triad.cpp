#include "triad.hpp"
#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>

triad_kernel::triad_kernel(
    triad::size_type num_entries)
    : Kernel()
    , num_entries(num_entries)
{
}

triad_kernel::~triad_kernel()
{
}

void triad_kernel::init(
    std::ostream & o,
    bool verbose)
{
    try {
        a = triad::value_array_type(num_entries, 1.0);
        b = triad::value_array_type(num_entries, 0.0);
        c = triad::value_array_type(num_entries, 0.0);
    } catch (std::system_error & e) {
        throw kernel_error(e.what());
    }
}

replacement::MemoryReferenceString triad_kernel::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads,
    int cache_line_size) const
{
    auto const & thread_affinities = trace_config.thread_affinities();
    auto const & numa_domains = trace_config.numa_domains();

    std::vector<int> numa_domain_affinity(thread_affinities.size(), 0);
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        auto numa_domain = thread_affinities[i].numa_domain;
        auto numa_domain_it = std::find(
            std::cbegin(numa_domains), std::cend(numa_domains), numa_domain);
        auto index = std::distance(std::cbegin(numa_domains), numa_domain_it);
        numa_domain_affinity[i] = index;
    }

    auto w = std::vector<std::pair<uintptr_t, int>>(3 * num_entries);
    for (triad::size_type k = 0, l = 0; k < num_entries; ++k, l += 3) {
        w[l] = std::make_pair(
            uintptr_t(&b[k]) / cache_line_size,
            numa_domain_affinity[thread]);
        w[l+1] = std::make_pair(
            uintptr_t(&c[k]) / cache_line_size,
            numa_domain_affinity[thread]);
        w[l+2] = std::make_pair(
            uintptr_t(&a[k]) / cache_line_size,
            numa_domain_affinity[thread]);
    }
    return w;
}

std::string triad_kernel::name() const
{
    return "triad";
}

std::ostream & triad_kernel::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << "triad" << '"' << ',' << '\n'
        << '"' << "num_entries" << '"' << ": " << '"' << num_entries << '"' << ',' << '\n'
        << "\n}";
}
