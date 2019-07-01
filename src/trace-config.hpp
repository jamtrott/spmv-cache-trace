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
          std::vector<std::string> parents,
          std::vector<std::string> events);

    std::string name;
    cache_size_type size;
    cache_size_type line_size;
    std::vector<std::string> parents;
    std::vector<std::string> events;
};

std::ostream & operator<<(
    std::ostream & o,
    Cache const & trace_config);

class ThreadAffinity
{
public:
    ThreadAffinity(
        int thread,
        int cpu,
        std::string const & cache,
        std::string const & numa_domain,
        std::vector<std::vector<std::string>> const & event_groups);

    int thread;
    int cpu;
    std::string cache;
    std::string numa_domain;
    std::vector<std::vector<std::string>> event_groups;
};

class TraceConfig
{
public:
    TraceConfig();
    TraceConfig(
        std::string const & name,
        std::string const & description,
        std::map<std::string, Cache> const & caches,
        std::vector<std::string> const & numa_domains,
        std::vector<ThreadAffinity> const & thread_affinities);
    ~TraceConfig();

    std::string const & name() const;
    std::string const & description() const;
    std::map<std::string, Cache> const & caches() const;
    std::vector<std::string> const & numa_domains() const;
    std::vector<ThreadAffinity> const & thread_affinities() const;

private:
    std::string name_;
    std::string description_;
    std::map<std::string, Cache> caches_;
    std::vector<std::string> numa_domains_;
    std::vector<ThreadAffinity> thread_affinities_;
};

TraceConfig read_trace_config(std::string const & path);

std::ostream & operator<<(
    std::ostream & o,
    TraceConfig const & trace_config);

#endif
