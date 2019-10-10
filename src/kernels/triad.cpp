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
    TraceConfig const & trace_config,
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

void triad_kernel::prepare(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    std::vector<int> cpus(num_threads, 0);
    for (int thread = 0; thread < num_threads; thread++)
        cpus[thread] = thread_affinities[thread].cpu;

    distribute_pages(a.data(), num_entries, num_threads, cpus.data());
    distribute_pages(b.data(), num_entries, num_threads, cpus.data());
    distribute_pages(c.data(), num_entries, num_threads, cpus.data());
}

void triad_kernel::run(TraceConfig const & trace_config)
{
    triad::value_type d = 3.1;
    #pragma omp for
    for (int i = 0; i < num_entries; i++)
        a[i] = b[i] + d * c[i];
}

replacement::MemoryReferenceString triad_kernel::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads) const
{
    auto const & thread_affinities = trace_config.thread_affinities();

    std::vector<int> numa_domain_affinity(thread_affinities.size(), 0);
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        numa_domain_affinity[i] = thread_affinities[i].numa_domain;
    }

    triad::size_type entries_per_thread = (num_entries + num_threads - 1) / num_threads;
    triad::size_type thread_start_entry = std::min(num_entries, thread * entries_per_thread);
    triad::size_type thread_end_entry = std::min(num_entries, (thread + 1) * entries_per_thread);
    triad::size_type thread_num_entries = thread_end_entry - thread_start_entry;
    auto w = std::vector<std::pair<uintptr_t, int>>(3 * thread_num_entries);
    for (triad::size_type k = thread_start_entry, l = 0; k < thread_end_entry; ++k, l += 3) {
        w[l+0] = std::make_pair(uintptr_t(&b[k]), numa_domain_affinity[thread]);
        w[l+1] = std::make_pair(uintptr_t(&c[k]), numa_domain_affinity[thread]);
        w[l+2] = std::make_pair(uintptr_t(&a[k]), numa_domain_affinity[thread]);
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
        << '"' << "num_entries" << '"' << ": " << '"' << num_entries << '"'
        << "\n}";
}
