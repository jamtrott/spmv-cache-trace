#include "csr-matrix.hpp"
#include "matrix-market.hpp"
#include "matrix-error.hpp"

#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace std::literals::string_literals;

namespace csr_matrix
{

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , num_entries(0)
    , row_alignment(1)
    , row_ptr()
    , column_index()
    , value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    index_type row_alignment,
    size_array_type const & row_ptr,
    index_array_type const & column_index,
    value_array_type const & value)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
    , row_alignment(row_alignment)
    , row_ptr(row_ptr)
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
    return sizeof(decltype(row_ptr)::value_type) * row_ptr.size()
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
    size_type start_nonzero = row_ptr[start_row];
    size_type end_nonzero = row_ptr[end_row];
    size_type nonzeros = end_nonzero - start_nonzero;
    return nonzeros;
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
    size_type start_nonzero = row_ptr[start_row];
    size_type end_nonzero = row_ptr[end_row];
    size_type nonzeros = end_nonzero - start_nonzero;

    size_type num_references = 3 * nonzeros + 2 * rows + 1;
    auto w = std::vector<std::pair<uintptr_t, int>>(
        num_references, std::make_pair(0,0));
    size_type l = 0;
    w[l++] = std::make_pair(uintptr_t(&row_ptr[start_row]),
                            numa_domains[thread]);
    for (index_type i = start_row; i < end_row; ++i) {
        w[l++] = std::make_pair(
            uintptr_t(&row_ptr[i+1]),
            numa_domains[thread]);
        for (size_type k = row_ptr[i]; k < row_ptr[i+1]; ++k) {
            index_type j = column_index[k];
            w[l++] = std::make_pair(
                uintptr_t(&column_index[k]),
                numa_domains[thread]);
            w[l++] = std::make_pair(
                uintptr_t(&value[k]),
                numa_domains[thread]);
            int column_thread = thread_of_index<value_type>(
                x.data(), columns, j, num_threads, page_size);
            w[l++] = std::make_pair(
                uintptr_t(&x[j]),
                numa_domains[column_thread]);
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
        std::equal(
            std::begin(a.row_ptr),
            std::end(a.row_ptr),
            std::begin(b.row_ptr)) &&
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
             << x.row_ptr << ' '
             << x.column_index << ' '
             << x.value;
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m)
{
    return from_matrix_market_row_aligned(m, 1);
}

Matrix from_matrix_market_row_aligned(
    matrix_market::Matrix const & m,
    index_type row_alignment)
{
    if (m.format() != matrix_market::Format::coordinate)
        throw matrix::matrix_error("Expected matrix in coordinate format");

    // Sort the matrix entries
    matrix_market::Matrix m_sorted = sort_matrix_row_major(m);
    auto row_indices = m_sorted.row_indices();
    auto column_indices = m_sorted.column_indices();
    auto values = m_sorted.values_real();

    // Compute the length of each row, including padding for alignment
    size_array_type row_ptr(m.rows() + 1, 0);
    size_type k = 0;
    size_type l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (l < (size_type) row_indices.size() && row_indices[l]-1 == r) {
            l++;
            k++;
        }
        k = ((k + (row_alignment-1)) / row_alignment) * row_alignment;
        row_ptr[r+1] = k;
    }

    // Determine the column indices and values
    index_array_type column_indices_aligned(row_ptr[m.rows()], 0);
    value_array_type values_aligned(row_ptr[m.rows()], 0.0);
    k = 0;
    l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (l < (size_type) m.num_entries() && row_indices[l]-1 == r) {
            column_indices_aligned[k] = column_indices[l]-1;
            values_aligned[k] = values[l];
            ++k;
            ++l;
        }

        while (k < row_ptr[r+1]) {
            column_indices_aligned[k] = 0;
            values_aligned[k] = 0.0;
            ++k;
        }
    }

    return Matrix(
        m.rows(), m.columns(), m.num_entries(),
        row_alignment, row_ptr,
        column_indices_aligned, values_aligned);
}

csr_matrix::value_array_type operator*(
    csr_matrix::Matrix const & A,
    csr_matrix::value_array_type const & x)
{
    if (A.columns != (index_type) x.size()) {
        throw matrix::matrix_error(
            "Size mismatch: "s +
            "A.size()="s + (
                std::to_string(A.rows) + "x"s + std::to_string(A.columns)) + ", " +
            "x.size()=" + std::to_string(x.size()));
    }

    csr_matrix::value_array_type y(A.rows, 0.0);
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
