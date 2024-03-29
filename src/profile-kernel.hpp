#ifndef PROFILE_KERNEL_HPP
#define PROFILE_KERNEL_HPP

#include "kernels/kernel.hpp"
#include "util/perf-events.hpp"

#include <chrono>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

using profiling_clock = std::chrono::steady_clock;
using duration_type = profiling_clock::duration::rep;
using event_count_type = uint64_t;

class ProfilingEvent
{
public:
    ProfilingEvent(
        std::string const & name,
        int thread,
        std::vector<uint64_t> const & time_enabled,
        std::vector<uint64_t> const & time_running,
        std::vector<event_count_type> const & counts);

    std::string name;
    int thread;
    std::vector<uint64_t> time_enabled;
    std::vector<uint64_t> time_running;
    std::vector<event_count_type> counts;
};

struct EventGroupValues
{
    EventGroupValues(
        uint64_t time_enabled,
        uint64_t time_running,
        std::map<std::string, event_count_type> event_counts);

    uint64_t time_enabled;
    uint64_t time_running;
    std::map<std::string, event_count_type> event_counts;
};

class ProfilingRun
{
public:
    ProfilingRun();
    ProfilingRun(
        duration_type const & execution_time,
        std::vector<std::vector<EventGroupValues>> const & event_group_values_per_thread_per_event_group);

    duration_type execution_time;
    std::vector<std::vector<EventGroupValues>> event_group_values_per_thread_per_event_group;
};

class Profiling
{
public:
    Profiling(
        TraceConfig const & trace_config,
        Kernel const & kernel,
        std::vector<ProfilingRun> const & profiling_runs);
    ~Profiling();

    TraceConfig const & trace_config() const;
    Kernel const & kernel() const;
    std::vector<ProfilingRun> const & profiling_runs() const;
    std::vector<duration_type> const & execution_time() const;
    std::vector<ProfilingEvent> const & profiling_events() const;

private:
    TraceConfig const & trace_config_;
    Kernel const & kernel_;
    std::vector<ProfilingRun> profiling_runs_;
    std::vector<duration_type> execution_time_;
    std::vector<ProfilingEvent> profiling_events_;
};

Profiling profile_kernel(
    TraceConfig const & trace_config,
    Kernel & kernel,
    bool warmup,
    bool flush_caches,
    int runs,
    perf::libpfm_context const & libpfm_context,
    std::ostream & o,
    bool verbose);

std::ostream & operator<<(
    std::ostream & o,
    Profiling const & profiling);

#endif
