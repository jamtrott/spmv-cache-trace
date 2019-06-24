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

class Cache
{
public:
    Cache(std::string name,
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

class TraceConfig
{
public:
    TraceConfig();
    TraceConfig(
        std::map<std::string, Cache> const & caches,
        std::vector<std::string> const & thread_affinities);
    ~TraceConfig();

    std::map<std::string, Cache> const & caches() const;
    std::vector<std::string> const & thread_affinities() const;

private:
    std::map<std::string, Cache> caches_;
    std::vector<std::string> thread_affinities_;
};

TraceConfig read_trace_config(std::string const & path);

std::ostream & operator<<(
    std::ostream & o,
    TraceConfig const & trace_config);

#endif
