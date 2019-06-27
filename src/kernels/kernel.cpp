#include "kernel.hpp"

#include <ostream>
#include <string>
#include <stdexcept>

kernel_error::kernel_error(std::string const & s) throw()
    : std::runtime_error(s)
{
}

std::ostream & operator<<(
    std::ostream & o,
    Kernel const & kernel)
{
    return kernel.print(o);
}
