#include "coo-spmv.hpp"
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

coo_spmv_kernel::coo_spmv_kernel(
    std::string const & matrix_path)
    : Kernel()
    , matrix_path(matrix_path)
{
}

coo_spmv_kernel::~coo_spmv_kernel()
{
}

void coo_spmv_kernel::init(
    std::ostream & o,
    bool verbose)
{
    try {
        matrix_market::Matrix mm =
            matrix_market::load_matrix(matrix_path, o, verbose);
        A = coo_matrix::from_matrix_market(mm, coo_matrix::Order::general);
        x = coo_matrix::value_array_type(A.columns, 1.0);
        y = coo_matrix::value_array_type(A.rows, 0.0);
    }
    catch (matrix_market::matrix_market_error & e) {
        std::stringstream s;
        s << matrix_path << ": " << e.what();
        throw kernel_error(s.str());
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

void coo_spmv_kernel::prepare(TraceConfig const & trace_config)
{
    auto const & thread_affinities = trace_config.thread_affinities();
    int num_threads = thread_affinities.size();
    std::vector<int> cpus(num_threads, 0);
    for (int thread = 0; thread < num_threads; thread++)
        cpus[thread] = thread_affinities[thread].cpu;

    distribute_pages(A.row_index.data(), A.row_index.size(), cpus.data());
    distribute_pages(A.column_index.data(), A.column_index.size(), cpus.data());
    distribute_pages(A.value.data(), A.value.size(), cpus.data());
    distribute_pages(x.data(), x.size(), cpus.data());
    distribute_pages(y.data(), y.size(), cpus.data());
}

void coo_spmv_kernel::run()
{
    coo_matrix::spmv(A, x, y);
}

replacement::MemoryReferenceString coo_spmv_kernel::memory_reference_string(
    TraceConfig const & trace_config,
    int thread,
    int num_threads) const
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
        x, y, thread, num_threads,
        numa_domain_affinity.data());
}

std::string coo_spmv_kernel::name() const
{
    return "coo-spmv";
}

std::ostream & coo_spmv_kernel::print(
    std::ostream & o) const
{
    return o
        << "{\n"
        << '"' << "name" << '"' << ": " << '"' << name() << '"' << ',' << '\n'
        << '"' << "matrix_path" << '"' << ": " << '"' << matrix_path << '"' << ',' << '\n'
        << '"' << "matrix_format" << '"' << ": " << '"' << "coo" << '"' << ',' << '\n'
        << '"' << "rows" << '"' << ": "  << A.rows << ',' << '\n'
        << '"' << "columns" << '"' << ": "  << A.columns  << ',' << '\n'
        << '"' << "nonzeros" << '"' << ": "  << A.num_entries  << ',' << '\n'
        << '"' << "matrix_size" << '"' << ": "  << A.size()
        << "\n}";
}
