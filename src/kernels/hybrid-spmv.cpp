#include "hybrid-spmv.hpp"
#include "kernel.hpp"
#include "trace-config.hpp"

#include "cache-simulation/replacement.hpp"
#include "matrix/hybrid-matrix.hpp"
#include "matrix/matrix-error.hpp"
#include "matrix/matrix-market.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>

hybrid_spmv_kernel::hybrid_spmv_kernel(
    std::string const & matrix_path)
    : Kernel()
    , matrix_path(matrix_path)
{
}

hybrid_spmv_kernel::~hybrid_spmv_kernel()
{
}

void hybrid_spmv_kernel::init(
    TraceConfig const & trace_config,
    std::ostream & o,
    bool verbose)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();

    try {
        matrix_market::Matrix mm =
            matrix_market::load_matrix(matrix_path, o, verbose);
        A = hybrid_matrix::from_matrix_market(mm, false, o, verbose);
        x = hybrid_matrix::value_array_type(A.columns, 1.0);
        y = hybrid_matrix::value_array_type(A.rows, 0.0);

        size_t workspace_size;
        if (__builtin_mul_overflow(num_threads, A.rows, &workspace_size)) {
            throw matrix::matrix_error(
                "Failed to compute HYBRID SpMV: "
            "Integer overflow when computing workspace size");
        }
        workspace_size = num_threads * A.rows;
        workspace = hybrid_matrix::value_array_type(workspace_size, 0.0);
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

void hybrid_spmv_kernel::prepare(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    std::vector<int> cpus(num_threads, 0);
    for (int thread = 0; thread < num_threads; thread++)
        cpus[thread] = thread_affinities[thread].cpu;

    distribute_pages(A.ellpack_column_index.data(), A.ellpack_column_index.size(), num_threads, cpus.data());
    distribute_pages(A.ellpack_value.data(), A.ellpack_value.size(), num_threads, cpus.data());
    distribute_pages(A.coo_row_index.data(), A.coo_row_index.size(), num_threads, cpus.data());
    distribute_pages(A.coo_column_index.data(), A.coo_column_index.size(), num_threads, cpus.data());
    distribute_pages(A.coo_value.data(), A.coo_value.size(), num_threads, cpus.data());
    distribute_pages(x.data(), x.size(), num_threads, cpus.data());
    distribute_pages(y.data(), y.size(), num_threads, cpus.data());
    distribute_pages(workspace.data(), workspace.size(), num_threads, cpus.data());
}

void hybrid_spmv_kernel::run(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    hybrid_matrix::spmv(num_threads, A, x, y, workspace, 0);
}

replacement::MemoryReferenceString hybrid_spmv_kernel::memory_reference_string(
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

    return A.spmv_memory_reference_string(
        x, y, workspace, thread, num_threads,
        numa_domain_affinity.data(),
        page_size);
}

std::string hybrid_spmv_kernel::name() const
{
    return "hybrid-spmv";
}

std::ostream & hybrid_spmv_kernel::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << name() << '"' << ',' << '\n'
        << '"' << "matrix_path" << '"' << ": " << '"' << matrix_path << '"' << ',' << '\n'
        << '"' << "matrix_format" << '"' << ": " << '"' << "hybrid" << '"' << ',' << '\n'
        << '"' << "rows" << '"' << ": "  << A.rows << ',' << '\n'
        << '"' << "columns" << '"' << ": "  << A.columns  << ',' << '\n'
        << '"' << "nonzeros" << '"' << ": "  << A.num_entries  << ',' << '\n'
        << '"' << "matrix_size" << '"' << ": "  << A.size() << ',' << '\n' << ',' << '\n'
        << '"' << "x_size" << '"' << ": " << sizeof(hybrid_matrix::value_type) * A.columns << ',' << '\n'
        << '"' << "y_size" << '"' << ": " << sizeof(hybrid_matrix::value_type) * A.rows << ',' << '\n'
        << '"' << "ellpack_row_length" << '"' << ": " << A.ellpack_row_length << ',' << '\n'
        << '"' << "num_ellpack_entries" << '"' << ": " << A.num_ellpack_entries << ',' << '\n'
        << '"' << "num_coo_entries" << '"' << ": " << A.num_coo_entries
        << "\n}";
}
