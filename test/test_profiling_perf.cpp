#include "profiling/perf.hpp"

#include <gtest/gtest.h>

#include <sched.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std::literals::string_literals;


TEST(profiling_perf, count_cpu_cycles)
{
    perf::libpfm_context c;
    auto events = std::vector<std::string>{"PERF_COUNT_HW_CPU_CYCLES"s};
    auto event_group = perf::EventGroup(c, events, 0, -1);
    event_group.enable();
    event_group.disable();
    auto event_group_values = event_group.values();
    auto const & values = event_group_values.values;
    ASSERT_GT(event_group_values.time_enabled, 0);
    ASSERT_GT(event_group_values.time_running, 0);
    ASSERT_EQ(values.size(), 1u);
    ASSERT_GT(values.at("PERF_COUNT_HW_CPU_CYCLES"s), 0u);
}

TEST(profiling_perf, count_multiple_events)
{
    perf::libpfm_context c;
    auto events = std::vector<std::string>{
        "PERF_COUNT_HW_CPU_CYCLES"s,
        "PERF_COUNT_HW_CACHE_REFERENCES"s};
    auto event_group = perf::EventGroup(c, events, 0, -1);
    event_group.enable();
    event_group.disable();
    auto event_group_values = event_group.values();
    auto const & values = event_group_values.values;
    ASSERT_GT(event_group_values.time_enabled, 0);
    ASSERT_GT(event_group_values.time_running, 0);
    ASSERT_EQ(values.size(), 2u);
    ASSERT_GE(values.at("PERF_COUNT_HW_CPU_CYCLES"s), 0u);
    ASSERT_GE(values.at("PERF_COUNT_HW_CACHE_REFERENCES"s), 0u);
}

int accumulate(
    std::vector<int> const & b)
{
    auto a = 0.0;
    for (size_t i = 0; i < b.size(); i++)
        a += b[i];
    return a;
}

void copy(
    std::vector<int> & a,
    std::vector<int> const & b)
{
    for (size_t i = 0; i < a.size(); i++)
        a[i] = b[i];
}

TEST(profiling_perf, multiple_event_groups)
{
    perf::libpfm_context c;
    std::vector<perf::EventGroup> event_groups;
    event_groups.emplace_back(
        c,
        std::vector<std::string>{
            "PERF_COUNT_HW_CACHE_L1D:READ:ACCESS"s,
            "PERF_COUNT_HW_CACHE_L1D:READ:MISS"s},
        0, -1);
    event_groups.emplace_back(
        c,
        std::vector<std::string>{
            "PERF_COUNT_HW_CACHE_L1D:WRITE:ACCESS"s},
        0, -1);
    event_groups.emplace_back(
        c,
        std::vector<std::string>{
            "PERF_COUNT_HW_CACHE_LL:READ:ACCESS"s,
            "PERF_COUNT_HW_CACHE_LL:READ:MISS"s,
        },
        0, -1);
    event_groups.emplace_back(
        c,
        std::vector<std::string>{
            "PERF_COUNT_HW_CACHE_LL:WRITE:ACCESS"s,
            "PERF_COUNT_HW_CACHE_LL:WRITE:MISS"s,
        },
        0, -1);

    auto N = 10000000;
    auto b = std::vector<int>(N);
    std::fill(std::begin(b), std::end(b), 1);

    for (auto & event_group : event_groups)
        event_group.enable();
    auto a = accumulate(b);
    for (auto & event_group : event_groups)
        event_group.disable();
    std::vector<perf::EventGroupValues> event_group_values;
    for (auto const & event_group : event_groups)
        event_group_values.emplace_back(event_group.values());

    ASSERT_EQ(N, a);
    ASSERT_EQ(event_group_values.size(), 4u);
    ASSERT_GT(event_group_values[0].time_enabled, 0);
    ASSERT_GT(event_group_values[0].time_running, 0);
    ASSERT_EQ(event_group_values[0].values.size(), 2u);
    ASSERT_GT(event_group_values[0].values.at("PERF_COUNT_HW_CACHE_L1D:READ:ACCESS"s), 0u);
    ASSERT_GT(event_group_values[0].values.at("PERF_COUNT_HW_CACHE_L1D:READ:MISS"s), 0u);
    ASSERT_GT(event_group_values[1].time_enabled, 0);
    ASSERT_GT(event_group_values[1].time_running, 0);
    ASSERT_EQ(event_group_values[1].values.size(), 1u);
    ASSERT_GT(event_group_values[1].values.at("PERF_COUNT_HW_CACHE_L1D:WRITE:ACCESS"s), 0u);
    ASSERT_GT(event_group_values[2].time_enabled, 0);
    ASSERT_GE(event_group_values[2].time_running, 0);
    ASSERT_EQ(event_group_values[2].values.size(), 2u);
    ASSERT_GE(event_group_values[2].values.at("PERF_COUNT_HW_CACHE_LL:READ:ACCESS"s), 0u);
    ASSERT_GE(event_group_values[2].values.at("PERF_COUNT_HW_CACHE_LL:READ:MISS"s), 0u);
    ASSERT_GT(event_group_values[3].time_enabled, 0);
    ASSERT_GE(event_group_values[3].time_running, 0);
    ASSERT_EQ(event_group_values[3].values.size(), 2u);
    ASSERT_GE(event_group_values[3].values.at("PERF_COUNT_HW_CACHE_LL:WRITE:ACCESS"s), 0u);
    ASSERT_GE(event_group_values[3].values.at("PERF_COUNT_HW_CACHE_LL:WRITE:MISS"s), 0u);
}

TEST(profiling_perf, cpu_wide_monitoring)
{
    perf::libpfm_context c;
    std::vector<perf::EventGroup> event_groups;

    int cpu = sched_getcpu();
    event_groups.emplace_back(
        c,
        std::vector<std::string>{
            "PERF_COUNT_HW_CACHE_L1D:READ:ACCESS"s,
            "PERF_COUNT_HW_CACHE_L1D:READ:MISS"s},
        -1, cpu);

    auto N = 1000000;
    auto b = std::vector<int>(N);
    std::fill(std::begin(b), std::end(b), 1);

    for (auto & event_group : event_groups)
        event_group.enable();
    auto a = accumulate(b);
    for (auto & event_group : event_groups)
        event_group.disable();
    std::vector<perf::EventGroupValues> event_group_values;
    for (auto const & event_group : event_groups)
        event_group_values.emplace_back(event_group.values());

    ASSERT_EQ(N, a);
    ASSERT_EQ(event_group_values.size(), 1u);
    ASSERT_GT(event_group_values[0].time_enabled, 0);
    ASSERT_GT(event_group_values[0].time_running, 0);
    ASSERT_EQ(event_group_values[0].values.size(), 2u);
    ASSERT_GT(event_group_values[0].values.at("PERF_COUNT_HW_CACHE_L1D:READ:ACCESS"s), 0u);
    ASSERT_GT(event_group_values[0].values.at("PERF_COUNT_HW_CACHE_L1D:READ:MISS"s), 0u);
}
