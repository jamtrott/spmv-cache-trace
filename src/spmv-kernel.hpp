#ifndef SPMV_KERNEL_HPP
#define SPMV_KERNEL_HPP

#include "kernel.hpp"
#include "trace-config.hpp"

#include "cache-simulation/replacement.hpp"
#include "matrix/matrix.hpp"

#include <iosfwd>
#include <string>

class SpMV : public Kernel
{
public:
    SpMV(std::string const & matrix_path,
         matrix::MatrixFormat const matrix_format,
         std::ostream & o,
         bool verbose);
    ~SpMV();

    std::string const & matrix_path() const;
    matrix::MatrixFormat const & matrix_format() const;
    matrix::Matrix const & matrix() const;

    replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads,
        int cache_line_size) const override;

    std::ostream & print(
        std::ostream & o) const override;

private:
    std::string const matrix_path_;
    matrix::MatrixFormat const matrix_format_;
    matrix::Matrix A;
    matrix::Vector x;
    matrix::Vector y;
};

#endif
