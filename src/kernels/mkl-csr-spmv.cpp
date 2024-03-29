#include "mkl-csr-spmv.hpp"
#include "kernel.hpp"
#include "trace-config.hpp"

#include "cache-simulation/replacement.hpp"
#include "matrix/csr-matrix.hpp"
#include "matrix/matrix-error.hpp"
#include "matrix/matrix-market.hpp"

#include <ostream>
#include <sstream>
#include <string>

mkl_csr_spmv_kernel::mkl_csr_spmv_kernel(
    std::string const & matrix_path)
    : Kernel()
    , matrix_path(matrix_path)
{
}

mkl_csr_spmv_kernel::~mkl_csr_spmv_kernel()
{
}

void mkl_csr_spmv_kernel::init(
    TraceConfig const & trace_config,
    std::ostream & o,
    bool verbose)
{
    try {
        matrix_market::Matrix mm =
            matrix_market::load_matrix(matrix_path, o, verbose);
        A = csr_matrix::from_matrix_market(mm);
        x = csr_matrix::value_array_type(A.columns, 1.0);
        y = csr_matrix::value_array_type(A.rows, 0.0);
    } catch (matrix::matrix_error & e) {
        std::stringstream s;
        s << matrix_path << ": " << e.what();
        throw kernel_error(s.str());
    } catch (std::system_error & e) {
        std::stringstream s;
        s << matrix_path << ": " << e.what();
        throw kernel_error(s.str());
    }
}

void mkl_csr_spmv_kernel::prepare(
    TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    std::vector<int> cpus(num_threads, 0);
    for (int thread = 0; thread < num_threads; thread++)
        cpus[thread] = thread_affinities[thread].cpu;

    distribute_pages(A.row_ptr.data(), A.row_ptr.size(), num_threads, cpus.data());
    distribute_pages(A.column_index.data(), A.column_index.size(), num_threads, cpus.data());
    distribute_pages(A.value.data(), A.value.size(), num_threads, cpus.data());
    distribute_pages(x.data(), x.size(), num_threads, cpus.data());
    distribute_pages(y.data(), y.size(), num_threads, cpus.data());
}

void mkl_csr_spmv_kernel::run(TraceConfig const & trace_config)
{
    try {
        csr_matrix::spmv_mkl(A, x, y);
    } catch (matrix::matrix_error & e) {
        std::stringstream s;
        s << matrix_path << ": " << e.what();
        throw kernel_error(s.str());
    }
}

replacement::MemoryReferenceString mkl_csr_spmv_kernel::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads) const
{
    throw kernel_error(
        "mkl_csr_spmv_kernel::memory_reference_string(): Not implemented");
}

std::string mkl_csr_spmv_kernel::name() const
{
    return "mkl-csr-spmv";
}

std::ostream & mkl_csr_spmv_kernel::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << name() << '"' << ',' << '\n'
        << '"' << "matrix_path" << '"' << ": " << '"' << matrix_path << '"' << ',' << '\n'
        << '"' << "matrix_format" << '"' << ": " << '"' << "csr" << '"' << ',' << '\n'
        << '"' << "rows" << '"' << ": " << A.rows << ',' << '\n'
        << '"' << "columns" << '"' << ": " << A.columns << ',' << '\n'
        << '"' << "nonzeros" << '"' << ": " << A.num_entries << ',' << '\n'
        << '"' << "matrix_size" << '"' << ": " << A.size() << ',' << '\n'
        << '"' << "x_size" << '"' << ": " << sizeof(csr_matrix::value_array_type) * A.columns << ',' << '\n'
        << '"' << "y_size" << '"' << ": " << sizeof(csr_matrix::value_array_type) * A.rows
        << "\n}";
}
