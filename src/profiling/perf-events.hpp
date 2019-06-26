#ifndef PERF_EVENTS_HPP
#define PERF_EVENTS_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <vector>

namespace perf {

struct EventGroupValues
{
    pid_t pid;
    int cpu;
    uint64_t time_enabled;
    uint64_t time_running;
    std::map<std::string, uint64_t> values;
};

uint64_t event_value(
    EventGroupValues const & event_group_values,
    std::string const & event);

std::map<std::string, uint64_t> event_values(
    EventGroupValues const & event_group_values);

struct Event
{
    Event(
        std::string const & name,
        uint64_t id,
        uint64_t fd)
        : name(name)
        , id(id)
        , fd(fd)
    {
    }

    std::string name;
    uint64_t id;
    uint64_t fd;
};

class EventGroup
{
public:
    EventGroup(
        libpfm_context const & c,
        std::vector<std::string> const & event_names,
        pid_t pid,
        int cpu);
    EventGroup(
        libpfm_context const & c,
        std::vector<std::string> const & event_names);
    ~EventGroup();

    EventGroup(EventGroup const &) = delete;
    EventGroup & operator=(EventGroup const &) = delete;
    EventGroup(EventGroup &&) = default;
    EventGroup & operator=(EventGroup &&) = default;

    void enable();
    void disable();

    std::vector<Event> const & events() const;
    pid_t pid() const;
    int cpu() const;
    bool enabled() const;
    EventGroupValues values() const;

private:
    std::vector<Event> events_;
    pid_t pid_;
    int cpu_;
    bool enabled_;
};

}

std::ostream & operator<<(
    std::ostream & o,
    perf::Event const & event);

std::ostream & operator<<(
    std::ostream & o,
    std::vector<perf::Event> const & events);

std::ostream & operator<<(
    std::ostream & o,
    perf::EventGroup const & event_group);

std::ostream & operator<<(
    std::ostream & o,
    perf::EventGroupValues const & values);

#endif
