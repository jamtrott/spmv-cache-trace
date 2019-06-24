#include "trace-config.hpp"
#include "util/json.h"

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

Cache::Cache(std::string name,
             cache_size_type size,
             cache_size_type line_size,
             std::vector<std::string> parents)
    : name(name)
    , size(size)
    , line_size(line_size)
    , parents(parents)
{
    if (size % line_size != 0) {
        std::stringstream s;
        s << name << ": "
          << "Expected size (" << size << ") "
          << "to be a multiple of line_size (" << line_size << ")";
        throw trace_config_error(s.str());
    }
}

TraceConfig::TraceConfig()
    : caches_()
    , thread_affinities_()
{
}

TraceConfig::TraceConfig(
    std::map<std::string, Cache> const & caches,
    std::vector<std::string> const & thread_affinities)
    : caches_(caches)
    , thread_affinities_(thread_affinities)
{
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        if (caches.find(thread_affinities[i]) == caches.end()) {
            std::stringstream s;
            s << "Invalid thread affinity for thread " << i << ": "
              << "\"" << thread_affinities[i] << "\" is not a known cache";
            throw trace_config_error(s.str());
        }
    }
}

TraceConfig::~TraceConfig()
{
}

std::map<std::string, Cache> const & TraceConfig::caches() const
{
    return caches_;
}

std::vector<std::string> const & TraceConfig::thread_affinities() const
{
    return thread_affinities_;
}

std::map<std::string, Cache> parse_caches(const struct json * root)
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
                "Expected '\"cache\": {\"size\": ..., \"line_size\": ..., \"parent\": ...}");
        }

        std::string name(json_to_key(cache));
        struct json * cache_value = json_to_value(cache);
        struct json * size = json_object_get(cache_value, "size");
        if (!size || !json_is_number(size))
            throw trace_config_error("Expected \"size\": (number)");
        struct json * line_size = json_object_get(cache_value, "line_size");
        if (!line_size || !json_is_number(line_size))
            throw trace_config_error("Expected \"line_size\": (number)");
        struct json * parent = json_object_get(cache_value, "parent");
        if (!parent || !json_is_array(parent))
            throw trace_config_error("Expected \"parent\": (string or array)");

        std::vector<std::string> parents;
        for (struct json * p = json_array_begin(parent);
             p != json_array_end();
             p = json_array_next(parent))
        {
            if (!json_is_string(p))
                throw trace_config_error("Expected \"parent\": (array of strings)");
            parents.push_back(std::string(json_to_string(p)));
        }

        caches.emplace(
            name,
            Cache(name,
                  json_to_int(size),
                  json_to_int(line_size),
                  parents));
    }

    return caches;
}

std::vector<std::string> parse_thread_affinities(const struct json * root)
{
    struct json * json_thread_affinities = json_object_get(root, "thread_affinities");
    if (!json_thread_affinities || !json_is_array(json_thread_affinities))
        throw trace_config_error("Expected \"thread_affinities\" array");

    std::vector<std::string> thread_affinities;
    for (struct json * thread_affinity = json_array_begin(json_thread_affinities);
         thread_affinity != json_array_end();
         thread_affinity = json_array_next(thread_affinity))
    {
        if (!json_is_string(thread_affinity))
            throw trace_config_error("Expected \"thread_affinities\": (array of strings)");
        thread_affinities.push_back(
            std::string(json_to_string(thread_affinity)));
    }
    return thread_affinities;
}

TraceConfig parse_trace_config(const struct json * root)
{
    std::map<std::string, Cache> caches = parse_caches(root);
    std::vector<std::string> thread_affinities = parse_thread_affinities(root);
    return TraceConfig(caches, thread_affinities);
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

    std::unique_ptr<struct json, void(*)(struct json *)> root(json_parse(s.c_str()), &json_free);
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
    Cache const & cache)
{
    return o << '{'
             << '"' << "size" << '"' << ": " << cache.size << ',' << ' '
             << '"' << "line_size" << '"' << ": " << cache.line_size << ',' << ' '
             << '"' << "parents" << '"' << ": " << cache.parents
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
    for (; it != end; ++it)
    {
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
    TraceConfig const & trace_config)
{
    return o << '{' << '\n'
             << '"' << "caches" << '"' << ": "
             << trace_config.caches() << ',' << '\n'
             << '"' << "thread_affinities" << '"' << ": "
             << trace_config.thread_affinities()
             << '\n' << '}';
}
