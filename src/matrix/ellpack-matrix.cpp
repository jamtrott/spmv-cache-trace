#include "ellpack-matrix.hpp"
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

namespace ellpack_matrix
{

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , num_entries(0)
    , row_length(0)
    , column_index()
    , value()
    , skip_padding(false)
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    index_type row_length,
    index_array_type const & column_index,
    value_array_type const & value,
    bool skip_padding)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
    , row_length(row_length)
    , column_index(column_index)
    , value(value)
    , skip_padding(skip_padding)
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
    return sizeof(decltype(column_index)::value_type) * column_index.size();
}

size_type Matrix::num_padding_entries() const
{
    return value.size() - num_entries;
}

std::size_t Matrix::value_padding_size() const
{
    return sizeof(decltype(value)::value_type) * num_padding_entries();
}

std::size_t Matrix::index_padding_size() const
{
    return sizeof(decltype(column_index)::value_type) * num_padding_entries();
}

index_type Matrix::spmv_rows_per_thread(int thread, int num_threads) const
{
    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    index_type rows = end_row - start_row;
    return rows;
}

size_type Matrix::spmv_nonzeros_per_thread(int thread, int num_threads) const
{
    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    size_type start_nonzero = start_row * row_length;
    size_type end_nonzero = end_row * row_length;
    size_type nonzeros = end_nonzero - start_nonzero;
    return nonzeros;
}

int thread_of_column(
    index_type column,
    index_type num_columns,
    int num_threads)
{
    index_type columns_per_thread = (num_columns + num_threads - 1) / num_threads;
    for (int thread = 0 ; thread < num_threads; thread++) {
        index_type start_columns = std::min(num_columns, thread * columns_per_thread);
        index_type end_columns = std::min(num_columns, (thread + 1) * columns_per_thread);
        if (column >= start_columns && column < end_columns)
            return thread;
    }
    return num_threads-1;
}

std::vector<std::pair<uintptr_t, int>>
Matrix::spmv_memory_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    int thread,
    int num_threads,
    int const * numa_domains,
    int page_size) const
{
    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    index_type rows = end_row - start_row;
    size_type start_nonzero = start_row * row_length;
    size_type end_nonzero = end_row * row_length;
    size_type nonzeros = end_nonzero - start_nonzero;

    size_type num_references = 3 * nonzeros + 1 * rows;
    std::vector<std::pair<uintptr_t, int>> w(
        num_references, std::make_pair(0,0));
    size_type l = 0;
    for (index_type i = start_row; i < end_row; ++i) {
        for (size_type k = i * row_length; k < (i+1)*row_length; ++k) {
            index_type j = column_index[k];
            w[l++] = std::make_pair(
                uintptr_t(&column_index[k]),
                numa_domains[thread]);
            w[l++] = std::make_pair(
                uintptr_t(&value[k]),
                numa_domains[thread]);
            int thread = thread_of_index<value_type>(
                x.data(), columns, j, num_threads, page_size);
            w[l++] = std::make_pair(
                uintptr_t(&x[j]),
                numa_domains[thread]);
        }
        w[l++] = std::make_pair(
            uintptr_t(&y[i]),
            numa_domains[thread]);
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.num_entries == b.num_entries &&
        a.row_length == b.row_length &&
        std::equal(
            std::begin(a.column_index),
            std::end(a.column_index),
            std::begin(b.column_index)) &&
        std::equal(
            std::begin(a.value),
            std::end(a.value),
            std::begin(b.value));
}

template <typename T, typename allocator>
std::ostream & operator<<(
    std::ostream & o,
    std::vector<T, allocator> const & v)
{
    if (v.size() == 0u)
        return o << "[]";

    o << '[';
    std::copy(std::begin(v), std::end(v) - 1u,
              std::ostream_iterator<unsigned long>(o, " "));
    return o << v[v.size()-1u] << ']';
}

std::ostream & operator<<(std::ostream & o, Matrix const & x)
{
    return o << x.rows << ' ' << x.columns << ' '
             << x.num_entries << ' '
             << x.row_length << ' '
             << x.column_index << ' '
             << x.value;
}

Matrix from_matrix_market_default(
    matrix_market::Matrix const & m)
{
    return from_matrix_market(m, false);
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    bool skip_padding)
{
    if (m.format() != matrix_market::Format::coordinate)
        throw matrix::matrix_error("Expected matrix in coordinate format");

    // Compute the row length and number of entries (including padding)
    index_type rows = m.rows();
    index_type row_length = m.max_row_length();
    size_type num_entries = rows * row_length;

    // Sort the matrix entries
    matrix_market::Matrix m_sorted = sort_matrix_row_major(m);
    auto row_indices = m_sorted.row_indices();
    auto column_indices = m_sorted.column_indices();
    auto values = m_sorted.values_real();

    // Insert the values and column indices with the required padding
    index_array_type columns_ellpack(num_entries, 0);
    value_array_type values_ellpack(num_entries, 0.0);
    size_type k = 0;
    size_type l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (k < (size_type) m.num_entries() && row_indices[k] - 1 == r) {
            columns_ellpack[l] = column_indices[k] - 1;
            values_ellpack[l] = values[k];
            ++k;
            ++l;
        }
        while (l < (r + 1) * row_length) {
            columns_ellpack[l] = skip_padding
                ? std::numeric_limits<index_type>::max()
                : column_indices[k-1] - 1;
            values_ellpack[l] = 0.0;
            ++l;
        }
    }

    return Matrix(
        m.rows(), m.columns(), m.num_entries(),
        row_length, columns_ellpack, values_ellpack, skip_padding);
}

namespace
{

inline void ellpack_spmv_inner_loop(
    index_type i,
    index_type row_length,
    index_type const * j,
    value_type const * a,
    value_type const * x,
    value_type * y)
{
    index_type k, l;
    value_type z = 0.0;
    for (l = 0; l < row_length; ++l) {
        k = i * row_length + l;
        z += a[k] * x[j[k]];
    }
    y[i] += z;
}

inline void ellpack_spmv(
    index_type num_rows,
    index_type row_length,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y,
    index_type chunk_size)
{
    #pragma omp for nowait schedule(static, chunk_size)
    for (index_type i = 0; i < num_rows; ++i) {
        ellpack_spmv_inner_loop(i, row_length, column_index, value, x, y);
    }
}

inline void ellpack_spmv_inner_loop_skip_padding(
    index_type i,
    index_type row_length,
    index_type const * j,
    value_type const * a,
    value_type const * x,
    value_type * y)
{
    index_type k, l;
    value_type z = 0.0;
    for (l = 0; l < row_length; ++l) {
        k = i * row_length + l;
        if (j[k] == std::numeric_limits<index_type>::max())
            break;
        z += a[k] * x[j[k]];
    }
    y[i] += z;
}

void ellpack_spmv_skip_padding(
    index_type num_rows,
    index_type row_length,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y,
    index_type chunk_size)
{
    #pragma omp for nowait schedule(static, chunk_size)
    for (index_type i = 0; i < num_rows; ++i) {
        ellpack_spmv_inner_loop_skip_padding(i, row_length, column_index, value, x, y);
    }
}

}

void spmv(
    ellpack_matrix::Matrix const & A,
    ellpack_matrix::value_array_type const & x,
    ellpack_matrix::value_array_type & y,
    index_type chunk_size)
{
    if (chunk_size <= 0) {
#ifdef USE_OPENMP
        int num_threads = omp_get_num_threads();
#else
        int num_threads = 1;
#endif
        chunk_size = (A.rows + num_threads - 1) / num_threads;
    }

    if (!A.skip_padding) {
        ellpack_spmv(
            A.rows, A.row_length, A.column_index.data(),
            A.value.data(), x.data(), y.data(), chunk_size);
    } else {
        ellpack_spmv_skip_padding(
            A.rows, A.row_length, A.column_index.data(),
            A.value.data(), x.data(), y.data(), chunk_size);
    }
}

ellpack_matrix::value_array_type operator*(
    ellpack_matrix::Matrix const & A,
    ellpack_matrix::value_array_type const & x)
{
    if (A.columns != (index_type) x.size()) {
        throw matrix::matrix_error(
            "Size mismatch: "s +
            "A.size()="s + (
                std::to_string(A.rows) + "x"s + std::to_string(A.columns)) + ", " +
            "x.size()=" + std::to_string(x.size()));
    }

    ellpack_matrix::value_array_type y(A.rows, 0.0);
    spmv(A, x, y);
    return y;
}


index_type spmv_rows_per_thread(
    Matrix const & A,
    int thread,
    int num_threads)
{
    return A.spmv_rows_per_thread(thread, num_threads);
}

size_type spmv_nonzeros_per_thread(
    Matrix const & A,
    int thread,
    int num_threads)
{
    return A.spmv_nonzeros_per_thread(thread, num_threads);
}

}
