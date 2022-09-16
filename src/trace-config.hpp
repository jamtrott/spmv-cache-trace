#ifndef TRACE_CONFIG_HPP
#define TRACE_CONFIG_HPP

#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class trace_config_error
    : public std::runtime_error
{
public:
    trace_config_error(std::string const & message);
};

typedef int64_t cache_size_type;
typedef int numa_domain_type;

class Cache
{
public:
    Cache(std::string const & name,
          cache_size_type size,
          cache_size_type line_size,
          double bandwidth,
          std::vector<double> const & bandwidth_per_numa_domain,
          std::string const & cache_miss_event,
          std::string const & parent);

    std::string name;
    cache_size_type size;
    cache_size_type line_size;
    double bandwidth;
    std::vector<double> bandwidth_per_numa_domain;
    std::string cache_miss_event;
    std::string parent;
};

std::ostream & operator<<(
    std::ostream & o,
    Cache const & trace_config);

class EventGroup
{
public:
    EventGroup(
        int pid,
        int cpu,
        std::vector<std::string> const & events);

    int pid;
    int cpu;
    std::vector<std::string> events;
};

class ThreadAffinity
{
public:
    ThreadAffinity(
        int thread,
        int cpu,
        std::string const & cache,
        int numa_domain,
        std::vector<EventGroup> const & event_groups);

    int thread;
    int cpu;
    std::string cache;
    int numa_domain;
    std::vector<EventGroup> event_groups;
};

class TraceConfig
{
public:
    TraceConfig();
    TraceConfig(
        std::string const & name,
        std::string const & description,
        int num_numa_domains,
        std::vector<double> const & bandwidth_per_numa_domain,
        std::map<std::string, Cache> const & caches,
        std::vector<ThreadAffinity> const & thread_affinities);
    ~TraceConfig();

    std::string const & name() const;
    std::string const & description() const;
    int num_numa_domains() const;
    std::vector<double> const & bandwidth_per_numa_domain() const;
    std::map<std::string, Cache> const & caches() const;
    std::vector<ThreadAffinity> const & thread_affinities() const;
    cache_size_type max_cache_size() const;

private:
    std::string name_;
    std::string description_;
    int num_numa_domains_;
    std::vector<double> bandwidth_per_numa_domain_;
    std::map<std::string, Cache> caches_;
    std::vector<ThreadAffinity> thread_affinities_;
};

TraceConfig read_trace_config(std::string const & path);

std::ostream & operator<<(
    std::ostream & o,
    TraceConfig const & trace_config);

#endif
