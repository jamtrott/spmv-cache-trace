#ifndef MKL_CSR_SPMV_HPP
#define MKL_CSR_SPMV_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "matrix/csr-matrix.hpp"

#include <iosfwd>
#include <string>

class mkl_csr_spmv_kernel : public Kernel
{
public:
    mkl_csr_spmv_kernel(std::string const & matrix_path);
    ~mkl_csr_spmv_kernel();

    void init(std::ostream & o,
              bool verbose) override;
    void prepare(TraceConfig const & trace_config) override;
    void run() override;

    replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads) const override;

    std::string name() const override;

    std::ostream & print(
        std::ostream & o) const override;

private:
    std::string matrix_path;
    csr_matrix::Matrix A;
    csr_matrix::value_array_type x;
    csr_matrix::value_array_type y;
};

#endif
