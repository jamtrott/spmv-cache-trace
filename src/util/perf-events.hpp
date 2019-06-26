#ifndef PERF_EVENTS_HPP
#define PERF_EVENTS_HPP

#include <cstdint>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>

struct perf_event_attr;

namespace perf
{

class libpfm_context;
class EventGroup;
class Event;

class perf_error
    : public std::runtime_error
{
public:
    perf_error(std::string const & s) throw();
};

/*
 * A context for calls to the libpfm4 library.
 */
class libpfm_context
{
public:
    libpfm_context();
    ~libpfm_context();

    libpfm_context(libpfm_context const &) = delete;
    libpfm_context & operator=(libpfm_context const &) = delete;
    libpfm_context(libpfm_context &&) = delete;
    libpfm_context & operator=(libpfm_context &&) = delete;

    /* Create a group of hardware performance monitoring events. */
    EventGroup make_event_group(
        std::vector<std::string> const & event_names,
        pid_t pid,
        int cpu) const;

    /* Print an overview of available hardware performance events. */
    void print_perf_events(
        std::ostream & o) const;

private:
    mutable std::mutex m;

private:
    /* Create a hardware performance monitoring event. */
    Event make_event(
        pid_t pid,
        int cpu,
        std::string const & name,
        bool disabled,
        uint64_t read_format,
        int groupfd) const;

    /* Obtain an event encoding from libpfm. */
    perf_event_attr event_encoding(
        std::string const & name) const;
};

/*
 * A group of hardware performance events that may be scheduled
 * together onto a hardware performance monitoring unit.
 */
class EventGroup
{
public:
    EventGroup(
        pid_t pid,
        int cpu,
        int groupfd,
        std::map<std::string, Event> const & events);
    ~EventGroup();

    EventGroup(EventGroup const &) = delete;
    EventGroup & operator=(EventGroup const &) = delete;
    EventGroup(EventGroup &&) = default;
    EventGroup & operator=(EventGroup &&) = default;

    void enable();
    void disable();
    void update();

    pid_t pid() const;
    int cpu() const;
    std::map<std::string, Event> const & events() const;

    bool enabled() const;
    uint64_t time_enabled() const;
    uint64_t time_running() const;
    std::map<std::string, uint64_t> event_counts(
        bool extrapolate = true) const;
    uint64_t event_count(
        std::string const & event,
        bool extrapolate = true) const;

private:
    pid_t pid_;
    int cpu_;
    int groupfd_;
    std::map<std::string, Event> events_;
    bool enabled_;
    uint64_t time_enabled_;
    uint64_t time_running_;

private:
    int perf_event_group_read(
        int fd, char * buffer, size_t size);
};

/*
 * A hardware performance monitoring event.
 */
class Event
{
public:
    Event(std::string const & name, int fd, uint64_t id);

    std::string const & name() const;
    int fd() const;
    uint64_t id() const;
    uint64_t count(bool extrapolate = true) const;

private:
    std::string name_;
    int fd_;
    uint64_t id_;
    uint64_t count_;
    uint64_t extrapolated_count_;

public:
    friend class EventGroup;
};

}

#endif
