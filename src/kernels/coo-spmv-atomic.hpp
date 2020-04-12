#ifndef COO_SPMV_ATOMIC_HPP
#define COO_SPMV_ATOMIC_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "matrix/coo-matrix.hpp"

#include <iosfwd>
#include <string>

class coo_spmv_atomic_kernel : public Kernel
{
public:
    coo_spmv_atomic_kernel(std::string const & matrix_path);
    ~coo_spmv_atomic_kernel();

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
    coo_matrix::Matrix A;
    coo_matrix::value_array_type x;
    coo_matrix::value_array_type y;
};

#endif
