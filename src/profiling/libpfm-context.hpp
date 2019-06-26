#ifndef PROFILING_LIBPFM_CONTEXT_HPP
#define PROFILING_LIBPFM_CONTEXT_HPP

#include <iosfwd>
#include <mutex>

struct perf_event_attr;

namespace perf {

struct libpfm_context
{
    libpfm_context();
    ~libpfm_context();

    libpfm_context(libpfm_context const &) = delete;
    libpfm_context & operator=(libpfm_context const &) = delete;
    libpfm_context(libpfm_context &&) = delete;
    libpfm_context & operator=(libpfm_context &&) = delete;

    mutable std::mutex m;
};

void list_perf_events(
    libpfm_context const & c,
    std::ostream & o);

perf_event_attr event_encoding(
    libpfm_context const & c,
    std::string const & name);

}

#endif
