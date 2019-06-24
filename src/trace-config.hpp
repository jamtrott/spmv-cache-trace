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
          std::vector<std::string> parents);

    std::string name;
    cache_size_type size;
    cache_size_type line_size;
    std::vector<std::string> parents;
};

std::ostream & operator<<(
    std::ostream & o,
    Cache const & trace_config);

class ThreadAffinity
{
public:
    ThreadAffinity(
        int thread,
        std::string const & cache,
        std::string const & numa_domain);

    int thread;
    std::string cache;
    std::string numa_domain;
};

class TraceConfig
{
public:
    TraceConfig();
    TraceConfig(
        std::map<std::string, Cache> const & caches,
        std::vector<std::string> const & numa_domains,
        std::vector<ThreadAffinity> const & thread_affinities);
    ~TraceConfig();

    std::map<std::string, Cache> const & caches() const;
    std::vector<std::string> const & numa_domains() const;
    std::vector<ThreadAffinity> const & thread_affinities() const;

private:
    std::map<std::string, Cache> caches_;
    std::vector<std::string> numa_domains_;
    std::vector<ThreadAffinity> thread_affinities_;
};

TraceConfig read_trace_config(std::string const & path);

std::ostream & operator<<(
    std::ostream & o,
    TraceConfig const & trace_config);

#endif
