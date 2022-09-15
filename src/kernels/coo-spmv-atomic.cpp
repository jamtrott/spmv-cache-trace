#include "coo-spmv-atomic.hpp"
#include "kernel.hpp"
#include "trace-config.hpp"

#include "cache-simulation/replacement.hpp"
#include "matrix/coo-matrix.hpp"
#include "matrix/matrix-error.hpp"
#include "matrix/matrix-market.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>

coo_spmv_atomic_kernel::coo_spmv_atomic_kernel(
    std::string const & matrix_path)
    : Kernel()
    , matrix_path(matrix_path)
{
}

coo_spmv_atomic_kernel::~coo_spmv_atomic_kernel()
{
}

void coo_spmv_atomic_kernel::init(
    TraceConfig const & trace_config,
    std::ostream & o,
    bool verbose)
{
    try {
        matrix_market::Matrix mm =
            matrix_market::load_matrix(matrix_path, o, verbose);
        A = coo_matrix::from_matrix_market(mm);
        x = coo_matrix::value_array_type(A.columns, 1.0);
        y = coo_matrix::value_array_type(A.rows, 0.0);
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

void coo_spmv_atomic_kernel::prepare(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    std::vector<int> cpus(num_threads, 0);
    for (int thread = 0; thread < num_threads; thread++)
        cpus[thread] = thread_affinities[thread].cpu;

    distribute_pages(A.row_index.data(), A.row_index.size(), num_threads, cpus.data());
    distribute_pages(A.column_index.data(), A.column_index.size(), num_threads, cpus.data());
    distribute_pages(A.value.data(), A.value.size(), num_threads, cpus.data());
    distribute_pages(x.data(), x.size(), num_threads, cpus.data());
    distribute_pages(y.data(), y.size(), num_threads, cpus.data());
}

void coo_spmv_atomic_kernel::run(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    coo_matrix::spmv_atomic(num_threads, A, x, y);
}

replacement::MemoryReferenceString coo_spmv_atomic_kernel::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads) const
{
    auto const & thread_affinities = trace_config.thread_affinities();
#ifdef HAVE_LIBNUMA
    int page_size = numa_pagesize();
#else
    int page_size = 4096;
#endif

    std::vector<int> numa_domain_affinity(thread_affinities.size(), 0);
    for (size_t i = 0; i < thread_affinities.size(); i++) {
        numa_domain_affinity[i] = thread_affinities[i].numa_domain;
    }

    return A.spmv_atomic_memory_reference_string(
        x, y, thread, num_threads,
        numa_domain_affinity.data(),
        page_size);
}

std::string coo_spmv_atomic_kernel::name() const
{
    return "coo-spmv-atomic";
}

std::ostream & coo_spmv_atomic_kernel::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << name() << '"' << ',' << '\n'
        << '"' << "matrix_path" << '"' << ": " << '"' << matrix_path << '"' << ',' << '\n'
        << '"' << "matrix_format" << '"' << ": " << '"' << "coo" << '"' << ',' << '\n'
        << '"' << "rows" << '"' << ": " << A.rows << ',' << '\n'
        << '"' << "columns" << '"' << ": " << A.columns << ',' << '\n'
        << '"' << "nonzeros" << '"' << ": " << A.num_entries << ',' << '\n'
        << '"' << "matrix_size" << '"' << ": " << A.size() << ',' << '\n'
        << '"' << "x_size" << '"' << ": " << sizeof(coo_matrix::value_type) * A.columns << ',' << '\n'
        << '"' << "y_size" << '"' << ": " << sizeof(coo_matrix::value_type) * A.rows
        << "\n}";
}
