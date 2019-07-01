#include "perf-events.hpp"

#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

using namespace std::literals::string_literals;

namespace perf
{

perf_error::perf_error(std::string const & s) throw()
    : std::runtime_error(s)
{
}

libpfm_context::libpfm_context()
    : m()
{
    auto ret = pfm_initialize();
    if (ret != PFM_SUCCESS)
        throw perf_error("pfm_initialise: "s + pfm_strerror(ret));
}

libpfm_context::~libpfm_context()
{
    pfm_terminate();
}

EventGroup libpfm_context::make_event_group(
    std::vector<std::string> const & event_names,
    pid_t pid,
    int cpu) const
{
    std::map<std::string, Event> events;
    int groupfd = -1;
    if (!event_names.empty()) {
        uint64_t read_format =
            PERF_FORMAT_GROUP
            | PERF_FORMAT_ID
            | PERF_FORMAT_TOTAL_TIME_ENABLED
            | PERF_FORMAT_TOTAL_TIME_RUNNING;

        // Create the group leader
        Event event(
            make_event(
                pid, cpu, event_names[0], true,
                read_format, -1));
        events.emplace(event_names[0], std::move(event));
        groupfd = events.at(event_names[0]).fd();

        // Create the remaining events
        for (auto i = 1u; i < event_names.size(); ++i) {
            events.emplace(
                event_names[i],
                make_event(
                    pid, cpu, event_names[i], false,
                    read_format, groupfd));
        }
    }
    return EventGroup(pid, cpu, groupfd, events);
}

void libpfm_context::print_perf_events(
    std::ostream & o) const
{
    std::lock_guard<std::mutex> l(m);

    o << "Hardware performance counter events:" << '\n';
    for (auto pmu = PFM_PMU_NONE; pmu < PFM_PMU_MAX; pmu = pfm_pmu_t(int(pmu) + 1)) {
        pfm_pmu_info_t pmu_info;
        memset(&pmu_info, 0, sizeof(pmu_info));

        auto err = pfm_get_pmu_info(pmu, &pmu_info);
        if (err != PFM_SUCCESS || !pmu_info.is_present)
            continue;

        o << pmu_info.name << ": " << pmu_info.desc << '\n';

        for (auto event = pmu_info.first_event;
             event != -1;
             event = pfm_get_event_next(event)) {
            pfm_event_info_t event_info;
            memset(&event_info, 0, sizeof(event_info));
            err = pfm_get_event_info(event, PFM_OS_NONE, &event_info);
            if (err != PFM_SUCCESS) {
                std::stringstream s;
                s << "pfm_get_event_info("
                  << std::to_string(event) << ',' << ' '
                  << "PFM_OS_NONE" << ',' << ' '
                  << "&event_info" << ')' << ':' << ' '
                  << pfm_strerror(err);
                throw perf_error(s.str());
            }

            o << "  " << pmu_info.name << "::" << event_info.name << '\n'
              << "    " << event_info.desc << '\n';
        }
        o << '\n';
    }
}

Event libpfm_context::make_event(
    pid_t pid,
    int cpu,
    std::string const & name,
    bool disabled,
    uint64_t read_format,
    int groupfd) const
{
    struct perf_event_attr pe = event_encoding(name);
    pe.read_format = read_format;
    pe.disabled = disabled ? 1 : 0;
    int flags = 0;
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
    return Event(name, fd, id);
}

struct perf_event_attr libpfm_context::event_encoding(
    std::string const & name) const
{
    std::lock_guard<std::mutex> l(m);

    struct perf_event_attr pe;
    memset(&pe, 0u, sizeof(struct perf_event_attr));
    auto encoding = pfm_perf_encode_arg_t{
        &pe, nullptr, sizeof(pfm_perf_encode_arg_t), 0, 0, 0};

    auto ret = pfm_get_os_event_encoding(
        name.c_str(),
        PFM_PLM3,
        PFM_OS_PERF_EVENT,
        &encoding);
    if (ret != PFM_SUCCESS) {
        std::stringstream s;
        s << "pfm_get_os_event_encoding("
          << '"' << name << '"' << ',' << ' '
          << "PFM_PLM3" << ',' << ' '
          << "PFM_OS_PERF_EVENT" << ',' << ' '
          << "&encoding" << ')' << ':' << ' '
          << pfm_strerror(ret);
        throw perf_error(s.str());
    }
    return pe;
}

std::ostream & operator<<(
    std::ostream & o,
    Event const & e)
{
    return o << '(' << e.id() << ',' << ' '
             << '"' << e.name() << '"' << ')';
}

std::ostream & operator<<(
    std::ostream & o,
    std::map<std::string, Event> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << (*it).second << ',' << ' ';
    return o << (*it).second << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    EventGroup const & event_group)
{
    return o
        << '{'
        << '"' << "pid" << '"' << ':' << ' ' << event_group.pid() << ',' << ' '
        << '"' << "cpu" << '"' << ':' << ' ' << event_group.cpu() << ',' << ' '
        << '"' << "events" << '"' << ':' << ' ' << event_group.events()
        << '}';
}

EventGroup::EventGroup(
    pid_t pid,
    int cpu,
    int groupfd,
    std::map<std::string, Event> const & events)
    : pid_(pid)
    , cpu_(cpu)
    , groupfd_(groupfd)
    , events_(events)
    , enabled_(false)
    , time_enabled_(0)
    , time_running_(0)
{
}

EventGroup::~EventGroup()
{
    if (enabled_)
        disable();
    for (auto it = std::cbegin(events_); it != std::cend(events_); ++it) {
        Event const & event = (*it).second;
        close(event.fd());
    }
}

void EventGroup::enable()
{
    if (!events_.empty() && groupfd_ != -1) {
        int ret;
        ret = ioctl(groupfd_, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        if (ret < 0) {
            std::stringstream s;
            s << (*this) << ':' << ' '
              << "ioctl("
              << groupfd_ << ',' << ' '
              << "PERF_EVENT_IOC_RESET" << ',' << ' '
              << "PERF_IOC_FLAG_GROUP" << ')' << ':' << ' '
              << strerror(errno);
            throw perf_error(s.str());
        }
        ret = ioctl(groupfd_, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
        if (ret < 0) {
            std::stringstream s;
            s << (*this) << ':' << ' '
              << "ioctl("
              << groupfd_ << ',' << ' '
              << "PERF_EVENT_IOC_ENABLE" << ',' << ' '
              << "PERF_IOC_FLAG_GROUP" << ')' << ':' << ' '
              << strerror(errno);
            throw perf_error(s.str());
        }
    }
    enabled_ = true;
}

void EventGroup::disable()
{
    if (!events_.empty() && groupfd_ != -1) {

        int ret = ioctl(groupfd_, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
        if (ret < 0) {
            std::stringstream s;
            s << (*this) << ':' << ' '
              << "ioctl("
              << groupfd_ << ',' << ' '
              << "PERF_EVENT_IOC_DISABLE" << ',' << ' '
              << "PERF_IOC_FLAG_GROUP" << ')' << ':' << ' '
              << strerror(errno);
            throw perf_error(s.str());
        }
    }
    enabled_ = false;
}

int EventGroup::perf_event_group_read(
    int fd, char * buffer, size_t size)
{
    ssize_t bytes_read = read(fd, buffer, size);
    if (bytes_read == -1 || bytes_read == 0 || (size_t) bytes_read == size) {
        std::stringstream s;
        s << (*this) << ':' << ' '
          << "read("
          << fd << ',' << ' '
          << uintptr_t(&buffer) << ',' << ' '
          << size << ')' << ' '
          << "returned " << bytes_read;
        throw perf_error(s.str());
    }
    return bytes_read;
}

uint64_t extrapolate_event_count(
    uint64_t time_enabled,
    uint64_t time_running,
    uint64_t count)
{
    if (time_running == 0)
        return 0;
    double ratio = (double) time_enabled / (double) time_running;
    return (uint64_t)((double) count * ratio);
}

void EventGroup::update()
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

    if (events_.empty() || groupfd_ == -1)
        return;

    perf_event_group_read_format r;
    perf_event_group_read(groupfd_, (char *)&r, sizeof(r));
    if (r.n > MAX_EVENT_VALUES) {
        std::stringstream s;
        s << (*this) << ':' << ' '
          << "Expected fewer than " << MAX_EVENT_VALUES << " event values";
        throw perf_error(s.str());
    }

    time_enabled_ = r.time_enabled;
    time_running_ = r.time_running;
    for (uint64_t i = 0; i < r.n; ++i) {
        auto & value = r.values[i];
        bool found = false;
        for (auto it = std::begin(events_); it != std::end(events_); ++it) {
            Event & event = (*it).second;
            if (event.id() == value.id) {
                event.count_ = value.value;
                event.extrapolated_count_ = extrapolate_event_count(
                    time_enabled_, time_running_, value.value);
                found = true;
            }
        }

        if (!found) {
            std::stringstream s;
            s << (*this) << ':' << ' '
              << "Unknown event id (" << value.id << ")";
            throw perf_error(s.str());
        }
    }
}

pid_t EventGroup::pid() const
{
    return pid_;
}

int EventGroup::cpu() const
{
    return cpu_;
}

std::map<std::string, Event> const & EventGroup::events() const
{
    return events_;
}

bool EventGroup::enabled() const
{
    return enabled_;
}

uint64_t EventGroup::time_enabled() const
{
    return time_enabled_;
}

uint64_t EventGroup::time_running() const
{
    return time_running_;
}

std::map<std::string, uint64_t> EventGroup::event_counts(
    bool extrapolate) const
{
    std::map<std::string, uint64_t> counts;
    for (auto it = std::cbegin(events_); it != std::cend(events_); ++it) {
        std::string const & name = (*it).first;
        Event const & event = (*it).second;
        counts.emplace(name, event.count(extrapolate));
    }
    return counts;
}

uint64_t EventGroup::event_count(
    std::string const & event,
    bool extrapolate) const
{
    auto it = events_.find(event);
    if (it != std::cend(events_)) {
        Event const & event = (*it).second;
        return event.count(extrapolate);
    }
    return 0;
}

Event::Event(
    std::string const & name,
    int fd,
    uint64_t id)
    : name_(name)
    , fd_(fd)
    , id_(id)
    , count_(0)
    , extrapolated_count_(0)
{
}

std::string const & Event::name() const
{
    return name_;
}

int Event::fd() const
{
    return fd_;
}

uint64_t Event::id() const
{
    return id_;
}

uint64_t Event::count(bool extrapolate) const
{
    if (extrapolate)
        return extrapolated_count_;
    return count_;
}

}