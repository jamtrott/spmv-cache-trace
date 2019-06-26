#include "profiling/libpfm-context.hpp"
#include "profiling/perf-error.hpp"

#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>

#include <cstring>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace std::literals::string_literals;

namespace perf {

libpfm_context::libpfm_context()
    : m()
{
    auto ret = pfm_initialize();
    if (ret != PFM_SUCCESS) {
        std::stringstream s;
        s << "pfm_initialize: " << pfm_strerror(ret);
        throw perf_error(s.str());
    }
}

libpfm_context::~libpfm_context()
{
    pfm_terminate();
}

void list_perf_events(libpfm_context const & c, std::ostream & o)
{
    std::lock_guard<std::mutex> l(c.m);

    o << "Hardware performance counter events:" << '\n';
    for (auto pmu = PFM_PMU_NONE; pmu < PFM_PMU_MAX; pmu = pfm_pmu_t(int(pmu) + 1)) {
        pfm_pmu_info_t pmu_info;
        memset(&pmu_info, 0, sizeof(pmu_info));

        auto err = pfm_get_pmu_info(pmu, &pmu_info);
        if (err != PFM_SUCCESS || !pmu_info.is_present)
            continue;

        o << pmu_info.name << ": " << pmu_info.desc << '\n';

        for (auto event = pmu_info.first_event; event != -1; event = pfm_get_event_next(event)) {
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

struct libpfm_error_category : std::error_category
{
    char const * name() const noexcept override
    {
        return "libpfm_category";
    }

    std::string message(int ev) const override
    {
        return std::string(pfm_strerror(ev));
    }
};

const std::error_category & libpfm_category() noexcept
{
    static const libpfm_error_category category{};
    return category;
}

perf_event_attr event_encoding(
    libpfm_context const & c,
    std::string const & name)
{
    perf_event_attr pe;
    memset(&pe, 0u, sizeof(perf_event_attr));
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

}
