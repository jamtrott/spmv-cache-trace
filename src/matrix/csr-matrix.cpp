#include "csr-matrix.hpp"
#include "matrix-market.hpp"

#include <ostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <valarray>

using namespace std::literals::string_literals;

namespace csr_matrix
{

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , numEntries(0)
    , row_alignment(1)
    , row_ptr()
    , column_index()
    , value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type numEntries,
    index_type row_alignment,
    size_array_type const row_ptr,
    index_array_type const column_index,
    value_array_type const value)
    : rows(rows)
    , columns(columns)
    , numEntries(numEntries)
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

std::vector<uintptr_t> Matrix::spmv_memory_reference_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    int thread,
    int num_threads,
    int cache_line_size) const
{
    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    index_type rows = end_row - start_row;
    size_type start_nonzero = row_ptr[start_row];
    size_type end_nonzero = row_ptr[end_row];
    size_type nonzeros = end_nonzero - start_nonzero;

    size_type num_references = 3 * nonzeros + 2 * rows + 1;
    auto w = std::vector<uintptr_t>(num_references, 0ul);
    size_type l = 0;
    w[l++] = uintptr_t(&row_ptr[start_row]) / cache_line_size;
    for (index_type i = start_row; i < end_row; ++i) {
        w[l++] = uintptr_t(&row_ptr[i+1]) / cache_line_size;
        for (size_type k = row_ptr[i]; k < row_ptr[i+1]; ++k) {
            index_type j = column_index[k];
            w[l++] = uintptr_t(&column_index[k]) / cache_line_size;
            w[l++] = uintptr_t(&value[k]) / cache_line_size;
            w[l++] = uintptr_t(&x[j]) / cache_line_size;
        }
        w[l++] = uintptr_t(&y[i]) / cache_line_size;
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.numEntries == b.numEntries &&
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
             << x.numEntries << ' '
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
    if (m.header.format != matrix_market::Format::coordinate)
        throw std::invalid_argument("Expected matrix in coordinate format");
    auto const & size = m.size;
    auto entries = m.sortedCoordinateEntries(
        matrix_market::Matrix::Order::row_major);

    // Compute the length of each row, including padding for alignment
    size_array_type row_ptr(m.rows() + 1, 0);
    size_type k = 0;
    size_type l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (l < (size_type) entries.size() && entries[l].i - 1 == r) {
            l++;
            k++;
        }
        k = ((k + (row_alignment-1)) / row_alignment) * row_alignment;
        row_ptr[r+1] = k;
    }

    // Determine the column indices and values
    index_array_type columns(row_ptr[m.rows()], 0);
    value_array_type values(row_ptr[m.rows()], 0.0);
    k = 0;
    l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (l < (size_type) entries.size() && entries[l].i - 1 == r) {
            columns[k] = entries[l].j - 1;
            values[k] = entries[l].a;
            ++k;
            ++l;
        }

        while (k < row_ptr[r+1]) {
            columns[k] = 0;
            values[k] = 0.0;
            ++k;
        }
    }

    return Matrix(
        size.rows, size.columns, size.numEntries,
        row_alignment, row_ptr, columns, values);
}

csr_matrix::value_array_type operator*(
    csr_matrix::Matrix const & A,
    csr_matrix::value_array_type const & x)
{
    if (A.columns != (index_type) x.size()) {
        throw std::invalid_argument(
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
