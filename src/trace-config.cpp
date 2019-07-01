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

trace_config_error::trace_config_error(std::string const & message)
    : std::runtime_error(message)
{
}

Cache::Cache(std::string const & name,
             cache_size_type size,
             cache_size_type line_size,
             std::vector<std::string> parents,
             std::vector<std::string> events)
    : name(name)
    , size(size)
    , line_size(line_size)
    , parents(parents)
    , events(events)
{
    if (size % line_size != 0) {
        std::stringstream s;
        s << name << ": "
          << "Expected size (" << size << ") "
          << "to be a multiple of line_size (" << line_size << ")";
        throw trace_config_error(s.str());
    }
}

ThreadAffinity::ThreadAffinity(
    int thread,
    int cpu,
    std::string const & cache,
    std::string const & numa_domain,
    std::vector<std::vector<std::string>> const & event_groups)
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
    , caches_()
    , numa_domains_()
    , thread_affinities_()
{
}

TraceConfig::TraceConfig(
    std::string const & name,
    std::string const & description,
    std::map<std::string, Cache> const & caches,
    std::vector<std::string> const & numa_domains,
    std::vector<ThreadAffinity> const & thread_affinities)
    : name_(name)
    , description_(description)
    , caches_(caches)
    , numa_domains_(numa_domains)
    , thread_affinities_(thread_affinities)
{
    // Check that the cache hierarchy is sensible
    for (auto it = std::cbegin(caches); it != std::cend(caches); ++it) {
        std::string const & name = (*it).first;
        Cache const & cache = (*it).second;
        for (std::string const & parent : cache.parents) {
            if (caches.find(parent) != caches.end())
                break;
            if (std::find(std::cbegin(numa_domains), std::cend(numa_domains), parent)
                != std::cend(numa_domains))
                break;

            std::stringstream s;
            s << name << ": \"parents\": "
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

        auto numa_domain_it = std::find(
            std::cbegin(numa_domains), std::cend(numa_domains),
            thread_affinities[i].numa_domain);
        if (numa_domain_it == numa_domains.end()) {
            std::stringstream s;
            s << "\"thread_affinities\": " << i << ": "
              << "Expected a NUMA domain, "
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

std::map<std::string, Cache> const & TraceConfig::caches() const
{
    return caches_;
}

std::vector<std::string> const & TraceConfig::numa_domains() const
{
    return numa_domains_;
}

std::vector<ThreadAffinity> const & TraceConfig::thread_affinities() const
{
    return thread_affinities_;
}

std::map<std::string, Cache> parse_caches(
    const struct json * root)
{
    struct json * json_caches = json_object_get(root, "caches");
    if (!json_caches || !json_is_object(json_caches))
        throw trace_config_error("Expected \"caches\" object");

    std::map<std::string, Cache> caches;
    for (struct json * cache = json_object_begin(json_caches);
         cache != json_object_end();
         cache = json_object_next(cache))
    {
        if (!json_is_member(cache)) {
            throw trace_config_error(
                "Expected '\"cache\": "
                "{\"size\": ..., \"line_size\": ..., \"parents\": ...}");
        }

        std::string name(json_to_key(cache));
        struct json * cache_value = json_to_value(cache);
        struct json * size = json_object_get(cache_value, "size");
        if (!size || !json_is_number(size))
            throw trace_config_error("Expected \"size\": (number)");
        struct json * line_size = json_object_get(cache_value, "line_size");
        if (!line_size || !json_is_number(line_size))
            throw trace_config_error("Expected \"line_size\": (number)");

        struct json * json_parents = json_object_get(cache_value, "parents");
        if (!json_parents || !json_is_array(json_parents))
            throw trace_config_error("Expected \"parents\": (array of strings)");

        std::vector<std::string> parents;
        for (struct json * parent = json_array_begin(json_parents);
             parent != json_array_end();
             parent = json_array_next(parent))
        {
            if (!json_is_string(parent)) {
                throw trace_config_error(
                    "Expected \"parents\": (array of strings)");
            }
            parents.push_back(std::string(json_to_string(parent)));
        }

        struct json * json_events = json_object_get(cache_value, "events");
        if (json_events && !json_is_array(json_events))
            throw trace_config_error(
                name + std::string(": Expected \"events\": (array of strings)"));

        std::vector<std::string> events;
        for (struct json * json_event = json_events ? json_array_begin(json_events) : NULL;
             json_event != json_array_end();
             json_event = json_array_next(json_event))
        {
            if (!json_is_string(json_event)) {
                throw trace_config_error(
                    "Expected \"event\": (string)");
            }
            events.push_back(std::string(json_to_string(json_event)));
        }

        caches.emplace(
            name,
            Cache(name,
                  json_to_int(size),
                  json_to_int(line_size),
                  parents,
                  events));
    }

    return caches;
}

std::vector<std::string> parse_numa_domains(
    const struct json * root)
{
    struct json * json_numa_domains = json_object_get(
        root, "numa_domains");
    if (!json_numa_domains || !json_is_array(json_numa_domains))
        throw trace_config_error("Expected \"numa_domains\" array");

    std::vector<std::string> numa_domains;
    for (struct json * numa_domain = json_array_begin(json_numa_domains);
         numa_domain != json_array_end();
         numa_domain = json_array_next(numa_domain))
    {
        if (!json_is_string(numa_domain)) {
            throw trace_config_error(
                "Expected '\"numa_domains\": (array of strings)");
        }
        numa_domains.push_back(
            std::string(json_to_string(numa_domain)));
    }
    return numa_domains;
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
        if (!numa_domain || !json_is_string(numa_domain))
            throw trace_config_error("Expected \"numa_domain\": (string)");

        struct json * json_event_groups = json_object_get(
            thread_affinity, "event_groups");
        if (json_event_groups && !json_is_array(json_event_groups))
            throw trace_config_error("Expected \"event_groups\": (array)");

        std::vector<std::vector<std::string>> event_groups;
        for (struct json * json_event_group =
                 json_event_groups ? json_array_begin(json_event_groups) : NULL;
             json_event_group != json_array_end();
             json_event_group = json_array_next(json_event_group))
        {
            if (!json_is_array(json_event_group)) {
                throw trace_config_error(
                    "Expected \"event_group\": (array)");
            }

            std::vector<std::string> events;
            for (struct json * json_event = json_array_begin(json_event_group);
                 json_event != json_array_end();
                 json_event = json_array_next(json_event))
            {
                if (!json_is_string(json_event)) {
                    throw trace_config_error(
                        "Expected \"event\": (string)");
                }
                events.push_back(std::string(json_to_string(json_event)));
            }
            event_groups.push_back(events);
        }

        thread_affinities.push_back(
            ThreadAffinity(thread,
                           json_to_int(cpu),
                           json_to_string(cache),
                           json_to_string(numa_domain),
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

    std::map<std::string, Cache> caches =
        parse_caches(root);
    std::vector<std::string> numa_domains =
        parse_numa_domains(root);
    std::vector<ThreadAffinity> thread_affinities =
        parse_thread_affinities(root);

    return TraceConfig(
        name, description,
        caches, numa_domains,
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
    Cache const & cache)
{
    return o << '{'
             << '"' << "size" << '"' << ": " << cache.size << ',' << ' '
             << '"' << "line_size" << '"' << ": " << cache.line_size << ',' << ' '
             << '"' << "parents" << '"' << ": " << cache.parents << ',' << ' '
             << '"' << "events" << '"' << ": " << cache.events
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
    ThreadAffinity const & thread_affinity)
{
    return o << '{'
             << '"' << "cpu" << '"' << ": "
             << thread_affinity.cpu << ',' << ' '
             << '"' << "cache" << '"' << ": "
             << '"' << thread_affinity.cache << '"' << ',' << ' '
             << '"' << "numa_domain" << '"' << ": "
             << '"' << thread_affinity.numa_domain << '"' << ',' << ' '
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
             << '"' << "numa_domains" << '"' << ": "
             << trace_config.numa_domains() << ',' << '\n'
             << '"' << "caches" << '"' << ": "
             << trace_config.caches() << ',' << '\n'
             << '"' << "thread_affinities" << '"' << ": "
             << trace_config.thread_affinities()
             << '\n' << '}';
}
