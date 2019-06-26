#ifndef PERF_ERROR_HPP
#define PERF_ERROR_HPP

#include <stdexcept>
#include <string>

namespace perf
{

class perf_error
    : public std::runtime_error
{
public:
    perf_error(std::string const & s) throw();
};

}

#endif
