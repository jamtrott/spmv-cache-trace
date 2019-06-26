#include "perf-error.hpp"

namespace perf
{

perf_error::perf_error(std::string const & s) throw()
    : std::runtime_error(s)
{}

}
