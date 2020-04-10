#ifndef CACHE_TRACE_HPP
#define CACHE_TRACE_HPP

#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "kernels/kernel.hpp"

#include <iosfwd>
#include <map>
#include <string>

using cache_miss_type = replacement::cache_miss_type;

class CacheTrace
{
public:
    CacheTrace(TraceConfig const & trace_config,
               Kernel const & kernel,
               std::map<std::string, std::vector<std::vector<cache_miss_type>>> const & cache_misses);
    ~CacheTrace();

    TraceConfig const & trace_config() const;
    Kernel const & kernel() const;
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> const & cache_misses() const;

private:
    TraceConfig const & trace_config_;
    Kernel const & kernel_;
    std::map<std::string, std::vector<std::vector<cache_miss_type>>> const cache_misses_;
};

CacheTrace trace_cache_misses(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    bool warmup,
    bool verbose);

std::ostream & operator<<(
    std::ostream & o,
    CacheTrace const & cache_trace);

#endif
