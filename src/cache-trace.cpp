#include "cache-trace.hpp"
#include "trace-config.hpp"

#include <map>
#include <ostream>
#include <sstream>
#include <string>

CacheTrace::CacheTrace(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> const & cache_misses)
    : trace_config_(trace_config)
    , kernel_(kernel)
    , cache_misses_(cache_misses)
{
}

CacheTrace::~CacheTrace()
{
}

TraceConfig const & CacheTrace::trace_config() const
{
    return trace_config_;
}

Kernel const & CacheTrace::kernel() const
{
    return kernel_;
}

std::map<std::string, std::vector<std::vector<cache_miss_type>>> const &
CacheTrace::cache_misses() const
{
    return cache_misses_;
}

bool cache_has_ancestor(
    TraceConfig const & trace_config,
    Cache const & a,
    Cache const & b)
{
    if (a.name == b.name)
        return true;

    auto const & caches = trace_config.caches();
    for (std::string const & parent : a.parents) {
        auto it = caches.find(parent);
        if (it == caches.end())
            return false;

        Cache const & parent_cache = (*it).second;
        if (cache_has_ancestor(trace_config, parent_cache, b))
            return true;
    }
    return false;
}

std::vector<int> active_threads(
    TraceConfig const & trace_config,
    Cache const & cache)
{
    auto const & caches = trace_config.caches();
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();

    std::vector<int> threads;
    for (int i = 0; i < num_threads; i++) {
        auto it = caches.find(thread_affinities[i].cache);
        if (it == caches.end()) {
            std::stringstream s;
            s << "Invalid thread affinity for thread " << i;
            throw trace_config_error(s.str());
        }

        Cache const & thread_affinity = (*it).second;
        if (cache_has_ancestor(trace_config, thread_affinity, cache))
            threads.push_back(i);
    }
    return threads;
}

std::vector<std::vector<cache_miss_type>> trace_cache_misses_per_cache(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    Cache const & cache)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    auto const & numa_domains = trace_config.numa_domains();
    int num_threads = thread_affinities.size();
    replacement::numa_domain_type num_numa_domains = numa_domains.size();

    // Determine which threads are active for the given cache
    std::vector<int> threads = active_threads(
        trace_config, cache);
    int num_active_threads = threads.size();

    // Obtain the memory reference strings for each thread
    std::vector<replacement::MemoryReferenceString>
        memory_reference_strings(num_active_threads);
    for (int n = 0; n < num_active_threads; n++) {
        memory_reference_strings[n] =
            kernel.memory_reference_string(
                trace_config, threads[n], num_threads);
    }

    int num_cache_lines = (cache.size + (cache.line_size-1)) / cache.line_size;
    replacement::LRU replacement_algorithm(num_cache_lines, cache.line_size);
    std::vector<std::vector<cache_miss_type>> active_threads_cache_misses =
        replacement::trace_cache_misses(
            replacement_algorithm,
            memory_reference_strings,
            num_numa_domains);

    std::vector<std::vector<cache_miss_type>> cache_misses(
        num_threads, std::vector<cache_miss_type>(num_numa_domains, 0));
    for (int i = 0; i < num_active_threads; i++)
        cache_misses[threads[i]] = active_threads_cache_misses[i];
    return cache_misses;
}

CacheTrace trace_cache_misses(
    TraceConfig const & trace_config,
    Kernel const & kernel)
{
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> cache_misses;

    auto const & caches = trace_config.caches();
    for (auto it = std::cbegin(caches); it != std::cend(caches); ++it) {
        std::string name = (*it).first;
        Cache const & cache = (*it).second;

        std::vector<std::vector<cache_miss_type>>
            num_cache_misses_per_thread_per_numa_domain =
            trace_cache_misses_per_cache(
                trace_config, kernel, cache);
        cache_misses.emplace(
            cache.name,
            num_cache_misses_per_thread_per_numa_domain);
    }

    return CacheTrace(trace_config, kernel, cache_misses);
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<cache_miss_type> const & cache_misses)
{
    if (cache_misses.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(cache_misses);
    auto end = --std::cend(cache_misses);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<std::vector<cache_miss_type>> const & cache_misses)
{
    if (cache_misses.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(cache_misses);
    auto end = --std::cend(cache_misses);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> const & cache_misses)
{
    if (cache_misses.empty())
        return o << "{}";

    o << '{' << '\n';
    auto it = std::cbegin(cache_misses);
    auto end = --std::cend(cache_misses);
    for (; it != end; ++it)
    {
        auto const & cache_miss = *it;
        o << '"' << cache_miss.first << '"' << ": "
          << cache_miss.second << ",\n";
    }
    auto const & cache_miss = *it;
    o << '"' << cache_miss.first << '"' << ": "
      << cache_miss.second << '\n';
    return o << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    CacheTrace const & cache_trace)
{
    return o << '{' << '\n'
             << '"' << "trace-config" << '"' << ": "
             << cache_trace.trace_config() << ',' << '\n'
             << '"' << "kernel" << '"' << ": "
             << cache_trace.kernel() << ',' << '\n'
             << '"' << "cache_misses" << '"' << ": "
             << cache_trace.cache_misses()
             << '\n' << '}';
}
