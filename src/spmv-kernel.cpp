#include "spmv-kernel.hpp"
#include "kernel.hpp"
#include "trace-config.hpp"

#include "cache-simulation/replacement.hpp"
#include "matrix/matrix.hpp"
#include "matrix/matrix-market.hpp"

#include <algorithm>
#include <ostream>
#include <string>

SpMV::SpMV(std::string const & matrix_path,
           matrix::MatrixFormat const matrix_format,
           std::ostream & o,
           bool verbose)
    : Kernel()
    , matrix_path_(matrix_path)
    , matrix_format_(matrix_format)
    , A(from_matrix_market(
            matrix_market::load_matrix(
                matrix_path, o, verbose),
            matrix_format, o, verbose))
{
    x = matrix::make_vector(
        matrix_format, std::vector<double>(A.columns(), 1.0));
    y = matrix::make_vector(
        matrix_format, std::vector<double>(A.rows(), 0.0));
}

SpMV::~SpMV()
{
}

std::string const & SpMV::matrix_path() const
{
    return matrix_path_;
}

matrix::MatrixFormat const & SpMV::matrix_format() const
{
    return matrix_format_;
}

matrix::Matrix const & SpMV::matrix() const
{
    return A;
}

replacement::MemoryReferenceString SpMV::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads,
    int cache_line_size) const
{
    auto const & thread_affinities = trace_config.thread_affinities();
    auto const & numa_domains = trace_config.numa_domains();

    std::vector<int> numa_domain_affinity(thread_affinities.size(), 0);
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        auto numa_domain = thread_affinities[i].numa_domain;
        auto numa_domain_it = std::find(
            std::cbegin(numa_domains), std::cend(numa_domains), numa_domain);
        auto index = std::distance(std::cbegin(numa_domains), numa_domain_it);
        numa_domain_affinity[i] = index;
    }

    return A.spmv_memory_reference_string(
        x, y, thread, num_threads, cache_line_size,
        numa_domain_affinity.data());
}

std::ostream & SpMV::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << "spmv" << '"' << ',' << '\n'
        << '"' << "matrix_path" << '"' << ": " << '"' << matrix_path_ << '"' << ',' << '\n'
        << '"' << "matrix_format" << '"' << ": " << '"' << matrix_format_ << '"' << ',' << '\n'
        << '"' << "rows" << '"' << ": "  << A.rows()  << ',' << '\n'
        << '"' << "columns" << '"' << ": "  << A.columns()  << ',' << '\n'
        << '"' << "nonzeros" << '"' << ": "  << A.nonzeros()  << ',' << '\n'
        << '"' << "matrix_size" << '"' << ": "  << A.size()
        << "\n}";
}
