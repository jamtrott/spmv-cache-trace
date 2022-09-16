#include "trace-config.hpp"
#include "util/json.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <errno.h>
#include <string.h>

using namespace std::literals::string_literals;

trace_config_error::trace_config_error(std::string const & message)
    : std::runtime_error(message)
{
}

Cache::Cache(
    std::string const & name,
    cache_size_type size,
    cache_size_type line_size,
    double bandwidth,
    std::vector<double> const & bandwidth_per_numa_domain,
    std::string const & cache_miss_event,
    std::string const & parent)
    : name(name)
    , size(size)
    , line_size(line_size)
    , bandwidth(bandwidth)
    , bandwidth_per_numa_domain(bandwidth_per_numa_domain)
    , cache_miss_event(cache_miss_event)
    , parent(parent)
{
    if (size % line_size != 0) {
        std::stringstream s;
        s << name << ": "
          << "Expected size (" << size << ") "
          << "to be a multiple of line_size (" << line_size << ")";
        throw trace_config_error(s.str());
    }
}

EventGroup::EventGroup(
    int pid,
    int cpu,
    std::vector<std::string> const & events)
    : pid(pid)
    , cpu(cpu)
    , events(events)
{
}

ThreadAffinity::ThreadAffinity(
    int thread,
    int cpu,
    std::string const & cache,
    int numa_domain,
    std::vector<EventGroup> const & event_groups)
    : thread(thread)
    , cpu(cpu)
    , cache(cache)
    , numa_domain(numa_domain)
    , event_groups(event_groups)
{
}

TraceConfig::TraceConfig()
    : name_()
    , description_()
    , num_numa_domains_()
    , bandwidth_per_numa_domain_()
    , caches_()
    , thread_affinities_()
{
}

TraceConfig::TraceConfig(
    std::string const & name,
    std::string const & description,
    int num_numa_domains,
    std::vector<double> const & bandwidth_per_numa_domain,
    std::map<std::string, Cache> const & caches,
    std::vector<ThreadAffinity> const & thread_affinities)
    : name_(name)
    , description_(description)
    , num_numa_domains_(num_numa_domains)
    , bandwidth_per_numa_domain_(bandwidth_per_numa_domain)
    , caches_(caches)
    , thread_affinities_(thread_affinities)
{
    // Check that the cache hierarchy is sensible
    for (auto it = std::cbegin(caches); it != std::cend(caches); ++it) {
        std::string const & name = (*it).first;
        Cache const & cache = (*it).second;
        std::string const & parent = cache.parent;
        if (!parent.empty() && caches.find(parent) == caches.end()) {
            std::stringstream s;
            s << name << ": \"parent\": "
              << "Expected a cache or numa domain, "
              << "got \"" << parent << "\"";
            throw trace_config_error(s.str());
        }
    }

    // Check that the thread affinities are sensible
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        if (caches.find(thread_affinities[i].cache) == caches.end()) {
            std::stringstream s;
            s << "\"thread_affinities\": " << i << ": "
              << "Expected a first-level cache, "
              << "got \"" << thread_affinities[i].cache << "\"";
            throw trace_config_error(s.str());
        }

        if (thread_affinities[i].numa_domain >= num_numa_domains_) {
            std::stringstream s;
            s << "\"thread_affinities\": " << i << ": "
              << "Expected a NUMA domain in the range [0," << num_numa_domains_ << "), "
              << "got \"" << thread_affinities[i].numa_domain << "\"";
            throw trace_config_error(s.str());
        }
    }
}

TraceConfig::~TraceConfig()
{
}

std::string const & TraceConfig::name() const
{
    return name_;
}

std::string const & TraceConfig::description() const
{
    return description_;
}

int TraceConfig::num_numa_domains() const
{
    return num_numa_domains_;
}

std::vector<double> const & TraceConfig::bandwidth_per_numa_domain() const
{
    return bandwidth_per_numa_domain_;
}

std::map<std::string, Cache> const & TraceConfig::caches() const
{
    return caches_;
}

std::vector<ThreadAffinity> const & TraceConfig::thread_affinities() const
{
    return thread_affinities_;
}

cache_size_type TraceConfig::max_cache_size() const
{
    cache_size_type cache_size = 0;
    for (auto it = std::cbegin(caches_); it != std::cend(caches_); ++it) {
        Cache const & cache = (*it).second;
        if (cache_size < cache.size)
            cache_size = cache.size;
    }
    return cache_size;
}

std::vector<double> parse_bandwidth_per_numa_domain(
    const struct json * json_bandwidth_per_numa_domain)
{
    std::vector<double> bandwidth_per_numa_domain;
    if (json_is_null(json_bandwidth_per_numa_domain))
        return bandwidth_per_numa_domain;

    for (struct json * bandwidth = json_array_begin(json_bandwidth_per_numa_domain);
         bandwidth != json_array_end();
         bandwidth = json_array_next(bandwidth))
    {
        if (!json_is_number(bandwidth)) {
            throw trace_config_error(
                "Expected '\"bandwidth_per_numa_domain\": "
                "to be an array of numbers");
        }

        bandwidth_per_numa_domain.push_back(
            json_to_double(bandwidth));
    }
    return bandwidth_per_numa_domain;
}

Cache parse_cache(
    const struct json * json_cache)
{
    if (!json_is_member(json_cache)) {
        throw trace_config_error(
            "Expected '\"cache\": "
            "{\"size\": ..., \"line_size\": ..., \"parents\": ...}");
    }

    std::string name(json_to_key(json_cache));
    struct json * cache_value = json_to_value(json_cache);

    struct json * size = json_object_get(cache_value, "size");
    if (!size || !json_is_number(size))
        throw trace_config_error("Expected \"size\": (number)");

    struct json * line_size = json_object_get(cache_value, "line_size");
    if (!line_size || !json_is_number(line_size))
        throw trace_config_error("Expected \"line_size\": (number)");

    struct json * bandwidth = json_object_get(cache_value, "bandwidth");
    if (!bandwidth || !(json_is_number(bandwidth) || json_is_null(bandwidth)))
        throw trace_config_error("Expected \"bandwidth\": (number) or null");

    struct json * bandwidth_per_numa_domain = json_object_get(cache_value, "bandwidth_per_numa_domain");
    if (!bandwidth_per_numa_domain || !(json_is_array(bandwidth_per_numa_domain) || json_is_null(bandwidth_per_numa_domain)))
        throw trace_config_error("Expected \"bandwidth_per_numa_domain\": (array) or null");
    std::vector<double> bandwidth_per_numa_domain_ = parse_bandwidth_per_numa_domain(bandwidth_per_numa_domain);

    struct json * cache_miss_event = json_object_get(cache_value, "cache_miss_event");
    if (!cache_miss_event || !(json_is_string(cache_miss_event) || json_is_null(cache_miss_event)))
        throw trace_config_error("Expected \"cache_miss_event\": (string) or null");

    struct json * parent = json_object_get(cache_value, "parent");
    if (!parent || !(json_is_string(parent) || json_is_null(parent)))
        throw trace_config_error("Expected \"parent\": (string) or null");

    return Cache(
        name,
        json_to_int(size),
        json_to_int(line_size),
        json_is_number(bandwidth) ? json_to_double(bandwidth) : 0.0,
        bandwidth_per_numa_domain_,
        json_is_string(cache_miss_event) ? json_to_string(cache_miss_event) : "",
        json_is_string(parent) ? json_to_string(parent) : "");
}

std::map<std::string, Cache> parse_caches(
    const struct json * root)
{
    struct json * json_caches = json_object_get(root, "caches");
    if (!json_caches || !json_is_object(json_caches))
        throw trace_config_error("Expected \"caches\" object");

    std::map<std::string, Cache> caches;
    for (struct json * json_cache = json_object_begin(json_caches);
         json_cache != json_object_end();
         json_cache = json_object_next(json_cache))
    {
        Cache cache = parse_cache(json_cache);
        caches.emplace(cache.name, cache);
    }
    return caches;
}

std::vector<ThreadAffinity> parse_thread_affinities(
    const struct json * root)
{
    struct json * json_thread_affinities = json_object_get(
        root, "thread_affinities");
    if (!json_thread_affinities || !json_is_array(json_thread_affinities))
        throw trace_config_error("Expected \"thread_affinities\" array");

    std::vector<ThreadAffinity> thread_affinities;
    int thread = 0;
    for (struct json * thread_affinity = json_array_begin(json_thread_affinities);
         thread_affinity != json_array_end();
         thread_affinity = json_array_next(thread_affinity))
    {
        if (!json_is_object(thread_affinity)) {
            throw trace_config_error(
                "Expected '\"thread_affinities\": "
                "{\"cache\": ..., \"numa_domain\": ...}");
        }

        struct json * cpu = json_object_get(thread_affinity, "cpu");
        if (!cpu || !json_is_number(cpu))
            throw trace_config_error("Expected \"cpu\": (number)");
        struct json * cache = json_object_get(thread_affinity, "cache");
        if (!cache || !json_is_string(cache))
            throw trace_config_error("Expected \"cache\": (string)");
        struct json * numa_domain = json_object_get(thread_affinity, "numa_domain");
        if (!numa_domain || !json_is_number(numa_domain))
            throw trace_config_error("Expected \"numa_domain\": (number)");

        struct json * json_event_groups = json_object_get(
            thread_affinity, "event_groups");
        if (json_event_groups && !json_is_array(json_event_groups))
            throw trace_config_error("Expected \"event_groups\": (array)");

        std::vector<EventGroup> event_groups;
        for (struct json * json_event_group =
                 json_event_groups ? json_array_begin(json_event_groups) : NULL;
             json_event_group != json_array_end();
             json_event_group = json_array_next(json_event_group))
        {
            if (!json_is_object(json_event_group)) {
                throw trace_config_error(
                    "Expected '\"event_groups\": "
                    "{\"pid\": ..., \"cpu\": ..., \"events\": ...}");
            }

            struct json * pid = json_object_get(json_event_group, "pid");
            if (!pid || !json_is_number(pid))
                throw trace_config_error("Expected \"pid\": (number)");
            struct json * cpu = json_object_get(json_event_group, "cpu");
            if (!cpu || !json_is_number(cpu))
                throw trace_config_error("Expected \"cpu\": (number)");
            struct json * json_events = json_object_get(json_event_group, "events");
            if (!json_events || !json_is_array(json_events))
                throw trace_config_error("Expected \"events\": (array)");

            std::vector<std::string> events;
            for (struct json * json_event = json_array_begin(json_events);
                 json_event != json_array_end();
                 json_event = json_array_next(json_event))
            {
                if (!json_is_string(json_event))
                    throw trace_config_error("Expected \"event\": (string)");
                events.push_back(std::string(json_to_string(json_event)));
            }
            event_groups.push_back(
                EventGroup(json_to_int(pid), json_to_int(cpu), events));
        }

        thread_affinities.push_back(
            ThreadAffinity(thread,
                           json_to_int(cpu),
                           json_to_string(cache),
                           json_to_int(numa_domain),
                           event_groups));
        thread++;
    }

    return thread_affinities;
}

TraceConfig parse_trace_config(const struct json * root)
{
    std::string name;
    struct json * json_name = json_object_get(
        root, "name");
    if (json_name && json_is_string(json_name))
        name = json_to_string(json_name);

    std::string description;
    struct json * json_description = json_object_get(
        root, "description");
    if (json_description && json_is_string(json_description))
        description = json_to_string(json_description);

    struct json * json_num_numa_domains = json_object_get(
        root, "num_numa_domains");
    int num_numa_domains = 0;
    if (json_num_numa_domains && json_is_number(json_num_numa_domains)) {
        num_numa_domains = json_to_int(json_num_numa_domains);
    }

    struct json * json_bandwidth_per_numa_domain = json_object_get(root, "bandwidth_per_numa_domain");
    std::vector<double> bandwidth_per_numa_domain;
    if (json_bandwidth_per_numa_domain && json_is_array(json_bandwidth_per_numa_domain)) {
        bandwidth_per_numa_domain = parse_bandwidth_per_numa_domain(json_bandwidth_per_numa_domain);
    }

    std::map<std::string, Cache> caches =
        parse_caches(root);
    std::vector<ThreadAffinity> thread_affinities =
        parse_thread_affinities(root);

    return TraceConfig(
        name,
        description,
        num_numa_domains,
        bandwidth_per_numa_domain,
        caches,
        thread_affinities);
}

TraceConfig read_trace_config(std::string const & path)
{
    std::ifstream ifs(path);
    if (!ifs)
        throw trace_config_error(strerror(errno));

    std::string s;
    ifs.seekg(0, std::ios::end);   
    s.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    s.assign(std::istreambuf_iterator<char>(ifs),
             std::istreambuf_iterator<char>());

    std::unique_ptr<struct json, void(*)(struct json *)> root(
        json_parse(s.c_str()), &json_free);
    if (json_is_invalid(root.get()))
        throw trace_config_error(json_to_error(root.get()));
    return parse_trace_config(root.get());
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<std::string> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << '"' << *it << '"' << ',' << ' ';
    return o << '"' << *it << '"' << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<std::vector<std::string>> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<double> const & xs)
{
    if (xs.empty())
        return o << "null";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    Cache const & cache)
{
    return o << '{'
             << '"' << "size" << '"' << ": " << cache.size << ',' << ' '
             << '"' << "line_size" << '"' << ": " << cache.line_size << ',' << ' '
             << '"' << "bandwidth" << '"' << ": " << (
                 (cache.bandwidth == 0.0) ? "null"s : std::to_string(cache.bandwidth)) << ',' << ' '
             << '"' << "bandwidth_per_numa_domain" << '"' << ": " << cache.bandwidth_per_numa_domain << ',' << ' '
             << '"' << "cache_miss_event" << '"' << ": " << (
                 cache.cache_miss_event.empty() ? "null"s : "\""s + cache.cache_miss_event + "\""s) << ',' << ' '
             << '"' << "parent" << '"' << ": " << (
                 cache.parent.empty() ? "null"s : "\""s + cache.parent + "\""s)
             << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    std::map<std::string, Cache> const & caches)
{
    if (caches.empty())
        return o << "{}";

    o << '{' << '\n';
    auto it = std::cbegin(caches);
    auto end = --std::cend(caches);
    for (; it != end; ++it) {
        auto const & cache = *it;
        o << '"' << cache.first << '"' << ": "
          << cache.second << ",\n";
    }
    auto const & cache = *it;
    o << '"' << cache.first << '"' << ": "
      << cache.second << '\n';
    return o << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    EventGroup const & eventgroup)
{
    return o << '{'
             << '"' << "pid" << '"' << ": "
             << eventgroup.pid << ',' << ' '
             << '"' << "cpu" << '"' << ": "
             << eventgroup.cpu << ',' << ' '
             << '"' << "events" << '"' << ": "
             << eventgroup.events
             << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<EventGroup> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[' << '\n';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << '\n';
    return o << *it << '\n' << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    ThreadAffinity const & thread_affinity)
{
    return o << '{'
             << '"' << "cpu" << '"' << ": "
             << thread_affinity.cpu << ',' << ' '
             << '"' << "cache" << '"' << ": "
             << '"' << thread_affinity.cache << '"' << ',' << ' '
             << '"' << "numa_domain" << '"' << ": "
             << thread_affinity.numa_domain << ',' << ' '
             << '"' << "event_groups" << '"' << ": "
             << thread_affinity.event_groups
             << '}';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<ThreadAffinity> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[' << '\n';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << '\n';
    return o << *it << '\n' << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<int> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<std::vector<int>> const & xs)
{
    if (xs.empty())
        return o << "[]";

    o << '[';
    auto it = std::cbegin(xs);
    auto end = --std::cend(xs);
    for (; it != end; ++it)
        o << *it << ',' << ' ';
    return o << *it << ']';
}

std::ostream & operator<<(
    std::ostream & o,
    TraceConfig const & trace_config)
{
    return o << '{' << '\n'
             << '"' << "name" << '"' << ": "
             << '"' << trace_config.name() << '"' << ',' << '\n'
             << '"' << "description" << '"' << ": "
             << '"' << trace_config.description() << '"' << ',' << '\n'
             << '"' << "num_numa_domains" << '"' << ": "
             << trace_config.num_numa_domains() << ',' << '\n'
             << '"' << "bandwidth_per_numa_domain" << '"' << ": "
             << trace_config.bandwidth_per_numa_domain() << ',' << '\n'
             << '"' << "caches" << '"' << ": "
             << trace_config.caches() << ',' << '\n'
             << '"' << "thread_affinities" << '"' << ": "
             << trace_config.thread_affinities()
             << '\n' << '}';
}
