#ifndef ELLPACK_SPMV_HPP
#define ELLPACK_SPMV_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "matrix/ellpack-matrix.hpp"

#include <iosfwd>
#include <string>

class ellpack_spmv_kernel : public Kernel
{
public:
    ellpack_spmv_kernel(std::string const & matrix_path);
    ~ellpack_spmv_kernel();

    void init(TraceConfig const & trace_config,
              std::ostream & o,
              bool verbose) override;
    void prepare(TraceConfig const & trace_config) override;
    void run(TraceConfig const & trace_config) override;

    replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads) const override;

    std::string name() const override;

    std::ostream & print(
        std::ostream & o) const override;

private:
    std::string matrix_path;
    ellpack_matrix::Matrix A;
    ellpack_matrix::value_array_type x;
    ellpack_matrix::value_array_type y;
};

#endif
