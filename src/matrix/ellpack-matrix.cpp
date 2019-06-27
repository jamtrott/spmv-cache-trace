#include "ellpack-matrix.hpp"
#include "matrix-market.hpp"
#include "matrix-error.hpp"

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
    : rows(0u)
    , columns(0u)
    , num_entries(0u)
    , rowLength(0u)
    , column_index()
    , value()
    , skip_padding(false)
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    index_type rowLength,
    index_array_type const & column_index,
    value_array_type const & value,
    bool skip_padding)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
    , rowLength(rowLength)
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

std::vector<std::pair<uintptr_t, int>> Matrix::spmv_memory_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_line_size) const
{
    auto w = std::vector<std::pair<uintptr_t, int>>{};
    for (index_type i = 0u; i < rows; ++i) {
        for (index_type l = 0u; l < rowLength; ++l) {
            size_type k = i * rowLength + l;
            index_type j = column_index[k];
            w.push_back(std::make_pair(uintptr_t(&x[j]) / cache_line_size, 0));
        }
        w.push_back(std::make_pair(uintptr_t(&y[i]) / cache_line_size, 0));
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.num_entries == b.num_entries &&
        a.rowLength == b.rowLength &&
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
std::ostream & operator<<(std::ostream & o, std::vector<T, allocator> const & v)
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
             << x.rowLength << ' '
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
    if (m.header.format != matrix_market::Format::coordinate)
        throw matrix::matrix_error("Expected matrix in coordinate format");
    auto const & size = m.size;

    // Compute the row length and number of entries (including padding)
    auto rows = size.rows;
    auto row_length = m.maxRowLength();
    auto num_entries = rows * row_length;
    auto entries = m.sortedCoordinateEntries(
        matrix_market::Matrix::Order::row_major);

    // Insert the values and column indices with the required padding
    index_array_type columns(num_entries, 0u);
    value_array_type values(num_entries, 0.0);
    size_type k = 0;
    size_type l = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        while (k < (size_type) entries.size() && entries[k].i - 1 == r) {
            columns[l] = entries[k].j - 1;
            values[l] = entries[k].a;
            ++k;
            ++l;
        }
        while (l < (r + 1) * row_length) {
            columns[l] = skip_padding
                ? std::numeric_limits<index_type>::max()
                : entries[k-1].j - 1;
            values[l] = 0.0;
            ++l;
        }
    }

    return Matrix(
        size.rows, size.columns, size.num_entries,
        row_length, columns, values, skip_padding);
}

namespace
{

void __attribute__ ((noinline)) ellpack_spmv(
    index_type const rows,
    index_type const rowLength,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y)
{
    index_type i, l;
    size_type k;
    for (i = 0; i < rows; ++i) {
        for (l = 0; l < rowLength; ++l) {
            k = i * rowLength + l;
            y[i] += value[k] * x[column_index[k]];
        }
    }
}

void ellpack_spmv_skip_padding(
    index_type const rows,
    index_type const rowLength,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y)
{
    index_type i, j, k;
    size_type l;
    value_type z;
    for (i = 0u; i < rows; ++i) {
        z = 0.0;
        for (j = 0u; j < rowLength; ++j) {
            l = i * rowLength + j;
            k = column_index[l];
            if (k == std::numeric_limits<index_type>::max())
                break;
            z += value[l] * x[k];
        }
        y[i] = z;
    }
}

}

void spmv(
    ellpack_matrix::Matrix const & A,
    ellpack_matrix::value_array_type const & x,
    ellpack_matrix::value_array_type & y)
{
    ellpack_spmv(A.rows, A.rowLength, A.column_index.data(),
                 A.value.data(), x.data(), y.data());
}

ellpack_matrix::value_array_type operator*(
    Matrix const & A,
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
    if (!A.skip_padding) {
        ellpack_spmv(
            A.rows, A.rowLength, A.column_index.data(),
            A.value.data(), x.data(), y.data());
    } else {
        ellpack_spmv_skip_padding(
            A.rows, A.rowLength, A.column_index.data(),
            A.value.data(), x.data(), y.data());
    }
    return y;
}

}
