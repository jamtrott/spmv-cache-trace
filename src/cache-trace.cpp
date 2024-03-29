#include "cache-trace.hpp"
#include "trace-config.hpp"

#include <map>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

CacheTrace::CacheTrace(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    bool warmup,
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> const & cache_misses)
    : trace_config_(trace_config)
    , kernel_(kernel)
    , warmup_(warmup)
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

bool CacheTrace::warmup() const
{
    return warmup_;
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
    std::string const & parent_name = a.parent;
    if (parent_name.empty())
        return false;

    auto it = caches.find(parent_name);
    if (it == caches.end())
        return false;

    Cache const & parent_cache = (*it).second;
    return cache_has_ancestor(trace_config, parent_cache, b);
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
    Cache const & cache,
    bool warmup,
    bool verbose,
    int progress_interval)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    replacement::numa_domain_type num_numa_domains = trace_config.num_numa_domains();

    // Determine which threads are active for the given cache
    std::vector<int> threads = active_threads(
        trace_config, cache);
    int num_active_threads = threads.size();
    if (num_active_threads <= 0) {
        return std::vector<std::vector<cache_miss_type>>();
    }

    // Obtain the memory reference strings for each thread
    std::vector<replacement::MemoryReferenceString>
        memory_reference_strings(num_active_threads);
    for (int n = 0; n < num_active_threads; n++) {
        if (verbose) {
            std::cerr << "Tracing memory accesses of kernel " << kernel.name()
                      << " for cache " << cache.name
                      << " (thread " << threads[n] << ")" << std::endl;
        }

        memory_reference_strings[n] =
            kernel.memory_reference_string(
                trace_config, threads[n], num_threads);
    }

    int num_cache_lines = (cache.size + (cache.line_size-1)) / cache.line_size;
    replacement::LRU replacement_algorithm(num_cache_lines, cache.line_size);
    if (warmup) {
        if (verbose) {
            std::cerr << "Simulating LRU cache replacement "
                      << "for cache " << cache.name << " (warmup run)" << std::endl;
        }

        replacement::trace_cache_misses(
            replacement_algorithm,
            memory_reference_strings,
            num_numa_domains,
            verbose,
            progress_interval);
    }

    if (verbose) {
        std::cerr << "Simulating LRU cache replacement "
                  << "for cache " << cache.name << std::endl;
    }

    std::vector<std::vector<cache_miss_type>> active_threads_cache_misses =
        replacement::trace_cache_misses(
            replacement_algorithm,
            memory_reference_strings,
            num_numa_domains,
            verbose,
            progress_interval);

    std::vector<std::vector<cache_miss_type>> cache_misses(
        num_threads, std::vector<cache_miss_type>(num_numa_domains, 0));
    for (int i = 0; i < num_active_threads; i++)
        cache_misses[threads[i]] = active_threads_cache_misses[i];
    return cache_misses;
}

CacheTrace trace_cache_misses(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    bool warmup,
    bool verbose,
    int progress_interval)
{
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> cache_misses;

    auto const & caches = trace_config.caches();
    for (auto it = caches.cbegin(); it != caches.cend(); ++it) {
        std::string name = (*it).first;
        Cache const & cache = (*it).second;

        std::vector<std::vector<cache_miss_type>>
            num_cache_misses_per_thread_per_numa_domain =
            trace_cache_misses_per_cache(
                trace_config, kernel, cache, warmup, verbose, progress_interval);
        cache_misses.emplace(
            cache.name,
            num_cache_misses_per_thread_per_numa_domain);
    }

    return CacheTrace(trace_config, kernel, warmup, cache_misses);
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<cache_miss_type> const & cache_misses)
{
    if (cache_misses.empty())
        return o << "[]";

    o << '[';
    auto it = cache_misses.cbegin();
    auto end = --cache_misses.cend();
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
    auto it = cache_misses.cbegin();
    auto end = --cache_misses.cend();
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
    auto it = cache_misses.cbegin();
    auto end = --cache_misses.cend();
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
             << '"' << "trace_config" << '"' << ": "
             << cache_trace.trace_config() << ',' << '\n'
             << '"' << "kernel" << '"' << ": "
             << cache_trace.kernel() << ',' << '\n'
             << '"' << "warmup" << '"' << ": "
             << (cache_trace.warmup()
                 ? std::string("true") : std::string("false")) << ',' << '\n'
             << '"' << "cache_misses" << '"' << ": "
             << cache_trace.cache_misses()
             << '\n' << '}';
}
