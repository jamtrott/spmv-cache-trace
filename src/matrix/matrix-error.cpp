#include "matrix-error.hpp"

namespace matrix
{

matrix_error::matrix_error(std::string const & s) throw()
    : std::runtime_error(s)
{}

}
