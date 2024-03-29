#include "profile-kernel.hpp"
#include "kernels/kernel.hpp"
#include "util/perf-events.hpp"
#include "util/sample.hpp"

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include <chrono>
#include <ostream>
#include <map>
#include <string>
#include <vector>

ProfilingEvent::ProfilingEvent(
    std::string const & name,
    int thread,
    std::vector<uint64_t> const & time_enabled,
    std::vector<uint64_t> const & time_running,
    std::vector<event_count_type> const & counts)
    : name(name)
    , thread(thread)
    , time_enabled(time_enabled)
    , time_running(time_running)
    , counts(counts)
{
}

ProfilingEvent make_profiling_event(
    std::string const & event,
    int thread,
    std::vector<ProfilingRun> const & profiling_runs)
{
    std::vector<uint64_t> time_enabled;
    std::vector<uint64_t> time_running;
    std::vector<event_count_type> counts;
    for (auto const & profiling_run : profiling_runs) {
        auto const & event_group_values_per_event_group =
            profiling_run.event_group_values_per_thread_per_event_group[thread];
        for (auto const & event_group_values : event_group_values_per_event_group) {
            auto it = event_group_values.event_counts.find(event);
            if (it != event_group_values.event_counts.end()) {
                time_enabled.push_back(event_group_values.time_enabled);
                time_running.push_back(event_group_values.time_running);
                counts.push_back((*it).second);
            }
        }
    }
    return ProfilingEvent(
        event, thread, time_enabled, time_running, counts);
}

EventGroupValues::EventGroupValues(
    uint64_t time_enabled,
    uint64_t time_running,
    std::map<std::string, event_count_type> event_counts)
    : time_enabled(time_enabled)
    , time_running(time_running)
    , event_counts(event_counts)
{
}

ProfilingRun::ProfilingRun()
    : execution_time()
    , event_group_values_per_thread_per_event_group()
{
}

ProfilingRun::ProfilingRun(
    duration_type const & execution_time,
    std::vector<std::vector<EventGroupValues>> const & event_group_values_per_thread_per_event_group)
    : execution_time(execution_time)
    , event_group_values_per_thread_per_event_group(
        event_group_values_per_thread_per_event_group)
{
}

Profiling::Profiling(
    TraceConfig const & trace_config,
    Kernel const & kernel,
    std::vector<ProfilingRun> const & profiling_runs)
    : trace_config_(trace_config)
    , kernel_(kernel)
    , profiling_runs_(profiling_runs)
{
    for (auto const & profiling_run : profiling_runs)
        execution_time_.push_back(profiling_run.execution_time);

    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();

    for (int thread = 0; thread < num_threads; thread++) {
        auto const & event_groups = thread_affinities[thread].event_groups;
        for (auto const & event_group : event_groups) {
            for (auto const & event : event_group.events) {
                profiling_events_.push_back(
                    make_profiling_event(event, thread, profiling_runs));
            }
        }
    }
}

Profiling::~Profiling()
{
}

TraceConfig const & Profiling::trace_config() const
{
    return trace_config_;
}

Kernel const & Profiling::kernel() const
{
    return kernel_;
}

std::vector<ProfilingRun> const & Profiling::profiling_runs() const
{
    return profiling_runs_;
}

std::vector<duration_type> const & Profiling::execution_time() const
{
    return execution_time_;
}

std::vector<ProfilingEvent> const & Profiling::profiling_events() const
{
    return profiling_events_;
}

/*
 * Profile a single run of a kernel by recording the execution time
 * and the given groups of hardware performance events.
 */
duration_type profile_kernel_run(
    TraceConfig const & trace_config,
    Kernel & kernel,
    std::vector<std::vector<perf::EventGroup>> & event_groups_per_thread)
{
    profiling_clock::time_point t0, t1;
    duration_type execution_time;
#ifdef USE_OPENMP
    int thread = omp_get_thread_num();
#else
    int thread = 0;
#endif

    #pragma omp barrier
    if ((size_t) thread < event_groups_per_thread.size()) {
        for (auto & event_group : event_groups_per_thread[thread]) {
            event_group.enable();
        }
    }
    #pragma omp master
    t0 = profiling_clock::now();

    #pragma omp barrier
    kernel.run(trace_config);
    #pragma omp barrier

    #pragma omp master
    t1 = profiling_clock::now();
    if ((size_t) thread < event_groups_per_thread.size()) {
        for (auto & event_group : event_groups_per_thread[thread]) {
            event_group.disable();
        }
    }
    if ((size_t) thread < event_groups_per_thread.size()) {
        for (auto & event_group : event_groups_per_thread[thread])
            event_group.update();
    }

    #pragma omp barrier

    execution_time = (t1 - t0).count();
    return execution_time;
}

void flush_cache(int cache_size)
{
    int N = 10*cache_size / sizeof(double);
    std::vector<double> b(N);
    #pragma omp for
    for (int i = 0; i < N; i++)
        b[i] = 1.1;
    volatile double x = 0.0;
    #pragma omp for
    for (int i = 0; i < N; i++)
        x += b[i];
}

/*
 * Perform multiple profiling runs for a kernel.
 */
Profiling profile_kernel(
    TraceConfig const & trace_config,
    Kernel & kernel,
    bool warmup,
    bool flush_caches,
    int runs,
    perf::libpfm_context const & libpfm_context,
    std::vector<std::vector<EventGroup>> const & eventgroups_per_thread,
    std::ostream & o,
    bool verbose)
{
    std::exception_ptr exception_ptr = nullptr;
    std::mutex m;

    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
#ifdef USE_OPENMP
    omp_set_num_threads(num_threads);
#else
    if (num_threads > 1) {
        throw trace_config_error(
            "Multi-threaded profiling failed: "
            "Please re-build with OpenMP enabled");
    }
#endif

    std::vector<std::vector<perf::EventGroup>> perf_event_groups_per_thread(
        eventgroups_per_thread.size());
    std::vector<ProfilingRun> profiling_runs(runs);

    #pragma omp parallel
    {
#ifdef USE_OPENMP
        int thread = omp_get_thread_num();
#else
        int thread = 0;
#endif
        int cpu = thread_affinities[thread].cpu;

        try {
#ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(cpu, &cpuset);
            if (sched_setaffinity(0, sizeof(cpuset), &cpuset) < 0) {
                throw std::system_error(
                    errno, std::generic_category(), "sched_setaffinity");
            }
#endif

            // Configure per-thread hardware performance counters
            if ((size_t) thread < eventgroups_per_thread.size()) {
                for (size_t event_group = 0;
                     event_group < eventgroups_per_thread[thread].size();
                     event_group++)
                {
                    perf_event_groups_per_thread[thread].emplace_back(
                        libpfm_context.make_event_group(
                            eventgroups_per_thread[thread][event_group].events,
                            eventgroups_per_thread[thread][event_group].pid,
                            cpu));
                }
            }

            // Initialise the kernel and perform a warm-up run
            kernel.prepare(trace_config);
            if (warmup)
                kernel.run(trace_config);

            for (int run = 0; run < runs; run++) {
                if (flush_caches)
                    flush_cache(trace_config.max_cache_size());

                duration_type execution_time = profile_kernel_run(
                    trace_config, kernel, perf_event_groups_per_thread);

                #pragma omp master
                {
                    std::vector<std::vector<EventGroupValues>>
                        event_group_values_per_thread_per_event_group(num_threads);
                    for (int thread = 0;
                         (size_t) thread < perf_event_groups_per_thread.size();
                         thread++)
                    {
                        for (size_t event_group = 0;
                             event_group < eventgroups_per_thread[thread].size();
                             event_group++)
                        {
                            event_group_values_per_thread_per_event_group[thread]
                                .emplace_back(
                                    perf_event_groups_per_thread[thread][event_group].time_enabled(),
                                    perf_event_groups_per_thread[thread][event_group].time_running(),
                                    perf_event_groups_per_thread[thread][event_group].event_counts());
                        }
                    }
                    profiling_runs[run] = ProfilingRun(
                        execution_time,
                        event_group_values_per_thread_per_event_group);
                }
            }
            #pragma omp barrier

        } catch (std::exception const &) {
            std::lock_guard<std::mutex> l(m);
            if (!exception_ptr)
                exception_ptr = std::current_exception();
        }
    }

    if (exception_ptr)
        std::rethrow_exception(exception_ptr);

    return Profiling(
        trace_config,
        kernel,
        profiling_runs);
}

Profiling profile_kernel(
    TraceConfig const & trace_config,
    Kernel & kernel,
    bool warmup,
    bool flush_caches,
    int runs,
    perf::libpfm_context const & libpfm_context,
    std::ostream & o,
    bool verbose)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();

    std::vector<std::vector<EventGroup>> eventgroups_per_thread;
    for (int thread = 0; thread < num_threads; thread++) {
        eventgroups_per_thread.push_back(
            thread_affinities[thread].event_groups);
    }

    return profile_kernel(
        trace_config, kernel, warmup, flush_caches,
        runs, libpfm_context, eventgroups_per_thread,
        o, verbose);
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<duration_type> const & v)
{
    return print_sample(o, v, std::string("ns"));
}

std::ostream & operator<<(
    std::ostream & o,
    ProfilingEvent const & e)
{
    o << '{' << '\n'
      << '"' << "name" << '"' << ": "
      << '"' << e.name << '"' << ',' << '\n'
      << '"' << "thread" << '"' << ": "
      << e.thread << ',' << '\n';
    o << '"' << "counts" << '"' << ": ";
    print_sample(o, e.counts, e.name);
    return o << '\n' << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<ProfilingEvent> const & event_counts)
{
    if (event_counts.empty())
        return o << "[]";

    o << '[' << '\n';
    auto it = std::cbegin(event_counts);
    auto end = --std::cend(event_counts);
    for (; it != end; ++it)
        o << *it << ',' << '\n';
    return o << *it << '\n' << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    Profiling const & profiling)
{
    return o
        << '{' << '\n'
        << '"' << "trace_config" << '"' << ": "
        << profiling.trace_config() << ',' << '\n'
        << '"' << "kernel" << '"' << ": "
        << profiling.kernel() << ',' << '\n'
        << '"' << "execution_time" << '"' << ": "
        << profiling.execution_time() << ',' << '\n'
        << '"' << "profiling_events" << '"' << ": "
        << profiling.profiling_events()
        << '\n' << '}';
}
