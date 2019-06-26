#include "profiling/perf.hpp"

#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace perf;
using namespace std::literals::string_literals;

namespace
{

std::pair<int, uint64_t> create_perf_event(
    libpfm_context const & c,
    std::string const & name,
    bool disabled,
    uint64_t read_format = 0u,
    int groupfd = -1,
    pid_t pid = 0,
    int cpu = -1)
{
    auto pe = event_encoding(c, name);
    pe.read_format = read_format;
    pe.disabled = disabled ? 1 : 0;
    auto flags = 0;
    int fd = perf_event_open(&pe, pid, cpu, groupfd, flags);
    if (fd < 0) {
        std::stringstream s;
        s << "perf_event_open("
          << "&pe" << ',' << ' '
          << pid << ',' << ' '
          << cpu << ',' << ' '
          << groupfd << ',' << ' '
          << flags << ')' << ':' << ' '
          << strerror(errno);
        throw perf_error(s.str());
    }

    uint64_t id;
    ioctl(fd, PERF_EVENT_IOC_ID, &id);
    return std::make_pair(fd, id);
}

int perf_event_group_read(
    int fd, char * buffer, std::size_t size)
{
    ssize_t bytes_read = read(fd, buffer, size);
    if (bytes_read == -1 || bytes_read == 0 || (size_t) bytes_read == size) {
        std::stringstream s;
        s << "read("
          << fd << ',' << ' '
          << uintptr_t(&buffer) << ',' << ' '
          << size << ')' << ' '
          << "returned " << bytes_read;
        throw perf_error(s.str());
    }
    return bytes_read;
}

}

namespace perf {

uint64_t event_value(
    EventGroupValues const & event_group_values,
    std::string const & event)
{
    if (event_group_values.time_running == 0)
        return 0;
    auto it = event_group_values.values.find(event);
    if (it == event_group_values.values.end())
        return 0;
    uint64_t value = (*it).second;
    double ratio =
        (double) event_group_values.time_running /
        (double) event_group_values.time_enabled;
    return (uint64_t)((double) value / ratio);
}

EventGroup::EventGroup(
    libpfm_context const & c,
    std::vector<std::string> const & event_names,
    pid_t pid,
    int cpu)
    : events_()
    , pid_(pid)
    , cpu_(cpu)
    , enabled_(false)
{
    std::lock_guard<std::mutex> l(c.m);
    if (event_names.empty())
        throw perf_error("Expected non-empty event group");

    uint64_t read_format =
        PERF_FORMAT_GROUP
        | PERF_FORMAT_ID
        | PERF_FORMAT_TOTAL_TIME_ENABLED
        | PERF_FORMAT_TOTAL_TIME_RUNNING;

    auto p = create_perf_event(
        c, event_names[0], true, read_format, -1, pid, cpu);
    auto groupfd = p.first;
    auto groupid = p.second;
    events_.emplace_back(event_names[0], groupid, groupfd);

    for (auto i = 1u; i < event_names.size(); ++i) {
        auto p = create_perf_event(
            c, event_names[i], false, read_format, groupfd, pid, cpu);
        auto fd = p.first;
        auto id = p.second;
        events_.emplace_back(event_names[i], id, fd);
    }
}

EventGroup::EventGroup(
    libpfm_context const & c,
    std::vector<std::string> const & event_names)
    : EventGroup(c, event_names, 0, -1)
{
}

EventGroup::~EventGroup()
{
    if (enabled_)
        disable();
    for (auto const & event : events_)
        close(event.fd);
}

void EventGroup::enable()
{
    ioctl(events_[0].fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(events_[0].fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    enabled_ = true;
}

void EventGroup::disable()
{
    ioctl(events_[0].fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    enabled_ = false;
}

std::vector<Event> const & EventGroup::events() const
{
    return events_;
}

pid_t EventGroup::pid() const
{
    return pid_;
}

int EventGroup::cpu() const
{
    return cpu_;
}

bool EventGroup::enabled() const
{
    return enabled_;
}

EventGroupValues EventGroup::values() const
{
#define MAX_EVENT_VALUES 10
    struct perf_event_group_read_format
    {
        uint64_t n;             /* The number of events */
        uint64_t time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
        uint64_t time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
        struct {
            uint64_t value;     /* The value of the event */
            uint64_t id;        /* if PERF_FORMAT_ID */
        } values[MAX_EVENT_VALUES];
    };

    perf_event_group_read_format r;
    perf_event_group_read(events_[0].fd, (char *)&r, sizeof(r));

    if (r.n > MAX_EVENT_VALUES) {
        std::stringstream s;
        s << events_[0].fd << ':' << ' '
          << "Expected fewer than " << MAX_EVENT_VALUES << " event values";
        throw perf_error(s.str());
    }

    EventGroupValues v{pid_, cpu_, r.time_enabled, r.time_running};
    for (uint64_t i = 0; i < r.n; ++i) {
        auto & value = r.values[i];

        auto it = std::find_if(
            std::cbegin(events_),
            std::cend(events_),
            [&] (auto const & e) { return e.id == value.id; });

        if (it == std::cend(events_)) {
            std::stringstream s;
            s << "Failed to read performance counter values for "s
              << "the performance event group "s << events_[0].fd << ": "s
              << "No event with ID "s << value.id << ". ("s
              << "n="s << r.n << ", "s
              << "time_enabled="s << r.time_enabled << ", "s
              << "time_running="s << r.time_running << ", "s
              << "r.values[" << i << "].id="s << value.id << ", "s
              << "r.values[" << i << "].value="s << value.value << ", "s
              << "events: "s << events_ << ")."s;
            throw perf_error(s.str());
        }

        auto const & event = *it;
        v.values.emplace(event.name, value.value);
    }
    return v;
}

}

std::ostream & operator<<(
    std::ostream & o,
    std::pair<std::string, uint64_t> const & event_value)
{
    auto p = event_value;
    auto name = p.first;
    auto value = p.second;
    return o << "        " << "\"" << name << "\": " << value;
}

std::ostream & operator<<(
    std::ostream & o,
    std::map<std::string, uint64_t> const & event_values)
{
    auto i = 0ul;
    auto size = event_values.size();
    for (auto const & x : event_values)
        o << x << ((++i < size) ? ",\n" : "");
    return o;
}

std::ostream & operator<<(
    std::ostream & o,
    perf::EventGroupValues const & values)
{
    return o << "{\n"
             << "        " << "\"pid\": " << values.pid << ",\n"
             << "        " << "\"cpu\": " << values.cpu << ",\n"
             << "        " << "\"time_enabled\": " << values.time_enabled << ",\n"
             << "        " << "\"time_running\": " << values.time_running << ",\n"
             << values.values << '\n'
             << "      " << "}";
}

std::ostream & operator<<(
    std::ostream & o,
    perf::Event const & event)
{
    return o
        << "name=" << event.name << ", "
        << "id=" << event.id << ", "
        << "fd=" << event.fd;
}


std::ostream & operator<<(
    std::ostream & o,
    std::vector<perf::Event> const & events)
{
    if (events.empty())
        return o;
    auto it = std::cbegin(events);
    auto end = std::prev(std::cend(events));
    for (; it != end; ++it)
        o << *it << ",\n";
    return o << *it;
}

std::ostream & operator<<(
    std::ostream & o,
    perf::EventGroup const & event_group)
{
    return o << "("
             << "events=" << event_group.events() << ", "
             << "pid=" << event_group.pid() << ", "
             << "cpu=" << event_group.cpu() << ", "
             << "enabled=" << event_group.enabled() << ")";
}
