#include "kernel.hpp"

#include <ostream>

std::ostream & operator<<(
    std::ostream & o,
    Kernel const & kernel)
{
    return kernel.print(o);
}
