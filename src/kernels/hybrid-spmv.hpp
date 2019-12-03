#ifndef HYBRID_SPMV_HPP
#define HYBRID_SPMV_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "matrix/hybrid-matrix.hpp"

#include <iosfwd>
#include <string>

class hybrid_spmv_kernel : public Kernel
{
public:
    hybrid_spmv_kernel(std::string const & matrix_path);
    ~hybrid_spmv_kernel();

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
    hybrid_matrix::Matrix A;
    hybrid_matrix::value_array_type x;
    hybrid_matrix::value_array_type y;
    hybrid_matrix::value_array_type workspace;
};

#endif
