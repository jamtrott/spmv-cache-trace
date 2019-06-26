#ifndef CSR_SPMV_HPP
#define CSR_SPMV_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "matrix/csr-matrix.hpp"

#include <iosfwd>
#include <string>

class csr_spmv_kernel : public Kernel
{
public:
    csr_spmv_kernel(std::string const & matrix_path);
    ~csr_spmv_kernel();

    void init(std::ostream & o,
              bool verbose) override;
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
