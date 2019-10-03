#include "coo-matrix.hpp"
#include "matrix-market.hpp"
#include "matrix-error.hpp"

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace std::literals::string_literals;

namespace coo_matrix
{

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , num_entries(0)
    , row_index()
    , column_index()
    , value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    index_array_type const & row_index,
    index_array_type const & column_index,
    value_array_type const & value)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
    , row_index(row_index)
    , column_index(column_index)
    , value(value)
{
}

std::size_t Matrix::size() const
{
    return value_size() + index_size();
}

std::size_t Matrix::value_size() const
{
    return sizeof(decltype(value)::value_type) * value.size();
}

std::size_t Matrix::index_size() const
{
    return sizeof(decltype(row_index)::value_type) * row_index.size()
        + sizeof(decltype(column_index)::value_type) * column_index.size();
}

size_type Matrix::num_padding_entries() const
{
    return 0;
}

std::size_t Matrix::value_padding_size() const
{
    return sizeof(decltype(value)::value_type) * num_padding_entries();
}

std::size_t Matrix::index_padding_size() const
{
    return sizeof(decltype(column_index)::value_type) * num_padding_entries()
        + sizeof(decltype(row_index)::value_type) * num_padding_entries();
}

std::vector<std::pair<uintptr_t, int>>
Matrix::spmv_memory_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    value_array_type const & workspace,
    int thread,
    int num_threads,
    int const * numa_domains,
    int page_size) const
{
    index_type num_entries_per_thread = (num_entries + num_threads - 1) / num_threads;
    index_type thread_start_entry = std::min(num_entries, thread * num_entries_per_thread);
    index_type thread_end_entry = std::min(num_entries, (thread + 1) * num_entries_per_thread);
    index_type thread_num_entries = thread_end_entry - thread_start_entry;

    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    index_type thread_num_rows = end_row - start_row;

    std::vector<std::pair<uintptr_t, int>> w(
        5 * thread_num_entries + 2 * thread_num_rows * num_threads,
        std::make_pair(0,0));
    size_t l = 0;
    for (size_type k = thread_start_entry; k < thread_end_entry; ++k, l += 5) {
        index_type i = row_index[k];
        index_type j = column_index[k];
        w[l+0] = std::make_pair(
            uintptr_t(&row_index[k]),
            numa_domains[thread]);
        w[l+1] = std::make_pair(
            uintptr_t(&column_index[k]),
            numa_domains[thread]);
        w[l+2] = std::make_pair(
            uintptr_t(&value[k]),
            numa_domains[thread]);
        int column_thread = thread_of_index<value_type>(
            x.data(), columns, j, num_threads, page_size);
        w[l+3] = std::make_pair(
            uintptr_t(&x[j]),
            numa_domains[column_thread]);
        w[l+4] = std::make_pair(
            uintptr_t(&workspace[thread*rows+i]),
            numa_domains[thread]);
    }

    for (index_type i = start_row; i < end_row; i++) {
        for (size_type j = 0; j < num_threads; j++, l += 2) {
            int t0 = thread_of_index<value_type>(
                workspace.data(), num_threads*thread_num_rows, j*rows+i,
                num_threads, page_size);
            w[l+0] =  std::make_pair(
                uintptr_t(&workspace[j*rows+i]),
                numa_domains[t0]);
            w[l+1] =  std::make_pair(
                uintptr_t(&y[i]),
                numa_domains[thread]);
        }
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.num_entries == b.num_entries &&
        a.row_index == b.row_index &&
        a.column_index == b.column_index &&
        a.value == b.value;
}

template <typename T, typename allocator>
std::ostream & operator<<(
    std::ostream & o,
    std::vector<T, allocator> const & v)
{
    if (v.size() == 0)
        return o << "[]";

    o << '[';
    std::copy(std::begin(v), std::end(v)-1u, std::ostream_iterator<T>(o, " "));
    return o << v[v.size()-1u] << ']';
}

std::ostream & operator<<(std::ostream & o, Matrix const & x)
{
    return o << x.rows << ' '
             << x.columns << ' '
             << x.num_entries << ' '
             << x.row_index << ' '
             << x.column_index << ' '
             << x.value;
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m)
{
    if (m.format() != matrix_market::Format::coordinate)
        throw matrix::matrix_error("Expected matrix in coordinate format");

    auto mm_row_indices = m.row_indices();
    index_array_type row_indices(m.num_entries());
    for (size_type k = 0; k < (size_type) m.num_entries(); k++)
        row_indices[k] = mm_row_indices[k] - 1;

    auto mm_column_indices = m.column_indices();
    index_array_type column_indices(m.num_entries());
    for (size_type k = 0; k < (size_type) m.num_entries(); k++)
        column_indices[k] = mm_column_indices[k] - 1;

    auto mm_values = m.values_real();
    value_array_type values(m.num_entries());
    for (size_type k = 0; k < (size_type) m.num_entries(); k++)
        values[k] = mm_values[k];

    return Matrix(m.rows(), m.columns(), m.num_entries(),
                  row_indices, column_indices, values);
}

namespace
{

void coo_spmv(
    int num_threads,
    index_type const num_rows,
    size_type const num_entries,
    index_type const * row_index,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y,
    value_type * workspace,
    index_type chunk_size)
{
#ifdef USE_OPENMP
    size_t thread = omp_get_thread_num();
#else
    size_t thread = 0;
#endif

    if (num_threads == 1) {
        for (size_type k = 0; k < num_entries; ++k) {
            y[row_index[k]] += value[k] * x[column_index[k]];
        }
    } else {
        /* Compute a partial result vector for each thread. */
        #pragma omp for schedule(static, chunk_size)
        for (size_type k = 0; k < num_entries; ++k) {
            workspace[thread*num_rows+row_index[k]] += value[k] * x[column_index[k]];
        }

        /* Perform a reduction step to combine the partial result vectors. */
        #pragma omp for schedule(static, chunk_size)
        for (index_type i = 0; i < num_rows; i++) {
            for (size_type j = 0; j < num_threads; j++) {
                y[i] += workspace[j*num_rows+i];
            }
        }
    }
}

}

void spmv(
    int num_threads,
    coo_matrix::Matrix const & A,
    coo_matrix::value_array_type const & x,
    coo_matrix::value_array_type & y,
    coo_matrix::value_array_type & workspace,
    coo_matrix::index_type chunk_size)
{
    if (chunk_size <= 0) {
        chunk_size = (A.num_entries + num_threads - 1) / num_threads;
    }

    coo_spmv(num_threads,
             A.rows,
             A.num_entries,
             A.row_index.data(),
             A.column_index.data(),
             A.value.data(),
             x.data(),
             y.data(),
             workspace.data(),
             chunk_size);
}

coo_matrix::value_array_type operator*(
    coo_matrix::Matrix const & A,
    coo_matrix::value_array_type const & x)
{
    if (A.columns != (index_type) x.size()) {
        throw matrix::matrix_error(
            "Size mismatch: "s +
            "A.size()="s + (
                std::to_string(A.rows) + "x"s + std::to_string(A.columns)) + ", " +
            "x.size()=" + std::to_string(x.size()));
    }

#ifdef USE_OPENMP
    int num_threads = omp_get_num_threads();
#else
    int num_threads = 1;
#endif

    coo_matrix::value_array_type y(A.rows, 0.0);
    coo_matrix::value_array_type workspace(num_threads*A.rows, 0.0);
    spmv(num_threads, A, x, y, workspace);
    return y;
}

}
