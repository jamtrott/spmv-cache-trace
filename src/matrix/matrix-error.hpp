#ifndef MATRIX_ERROR_HPP
#define MATRIX_ERROR_HPP

#include <stdexcept>
#include <string>

namespace matrix
{

class matrix_error
    : public std::runtime_error
{
public:
    matrix_error(std::string const & s) throw();
};

}

#endif
