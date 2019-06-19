#include "ellpack-matrix.hpp"
#include "matrix-market.hpp"

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
    , numEntries(0u)
    , rowLength(0u)
    , column_index()
    , value()
    , skip_padding(false)
{
}

Matrix::Matrix(
    unsigned int rows,
    unsigned int columns,
    unsigned int numEntries,
    unsigned int rowLength,
    index_array_type const & column_index,
    value_array_type const & value,
    bool skip_padding)
    : rows(rows)
    , columns(columns)
    , numEntries(numEntries)
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

unsigned int Matrix::num_padding_entries() const
{
    return value.size() - numEntries;
}

std::size_t Matrix::value_padding_size() const
{
    return sizeof(decltype(value)::value_type) * num_padding_entries();
}

std::size_t Matrix::index_padding_size() const
{
    return sizeof(decltype(column_index)::value_type) * num_padding_entries();
}

std::vector<uintptr_t> Matrix::spmv_page_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_line_size) const
{
    auto w = std::vector<uintptr_t>{};
    for (auto i = 0u; i < rows; ++i) {
        for (auto l = 0u; l < rowLength; ++l) {
            auto k = i * rowLength + l;
            auto j = column_index[k];
            w.push_back(uintptr_t(&x[j]) / cache_line_size);
        }
        w.push_back(uintptr_t(&y[i]) / cache_line_size);
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.numEntries == b.numEntries &&
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
             << x.numEntries << ' '
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
        throw std::invalid_argument("Expected matrix in coordinate format");
    auto const & size = m.size;

    // Compute the row length and number of entries (including padding)
    auto rows = size.rows;
    auto row_length = m.maxRowLength();
    auto numEntries = rows * row_length;
    auto entries = m.sortedCoordinateEntries(
        matrix_market::Matrix::Order::row_major);

    // Insert the values and column indices with the required padding
    Matrix::index_array_type columns(numEntries, 0u);
    Matrix::value_array_type values(numEntries, 0.0);
    auto k = 0u;
    auto l = 0u;
    for (auto r = 0u; r < m.rows(); ++r) {
        while (k < entries.size() && entries[k].i - 1u == r) {
            columns[l] = entries[k].j - 1u;
            values[l] = entries[k].a;
            ++k;
            ++l;
        }
        while (l < (r + 1u) * row_length) {
            columns[l] = skip_padding
                ? std::numeric_limits<unsigned int>::max()
                : entries[k-1u].j - 1u;
            values[l] = 0.0;
            ++l;
        }
    }

    return Matrix(
        size.rows, size.columns, size.numEntries,
        row_length, columns, values, skip_padding);
}

namespace
{

void __attribute__ ((noinline)) ellpack_spmv(
    unsigned int const rows,
    unsigned int const rowLength,
    unsigned int const * column_index,
    double const * value,
    double const * x,
    double * y)
{
    unsigned int i, k, l;
    for (i = 0u; i < rows; ++i) {
        for (l = 0u; l < rowLength; ++l) {
            k = i * rowLength + l;
            y[i] += value[k] * x[column_index[k]];
        }
    }
}

void ellpack_spmv_skip_padding(
    unsigned int const rows,
    unsigned int const rowLength,
    unsigned int const * column_index,
    double const * value,
    double const * x,
    double * y)
{
    unsigned int i, j, k, l;
    double z;
    for (i = 0u; i < rows; ++i) {
        z = 0.0;
        for (j = 0u; j < rowLength; ++j) {
            l = i * rowLength + j;
            k = column_index[l];
            if (k == std::numeric_limits<unsigned int>::max())
                break;
            z += value[l] * x[k];
        }
        y[i] = z;
    }
}

}

void spmv(
    ellpack_matrix::Matrix const & A,
    ellpack_matrix::Matrix::value_array_type const & x,
    ellpack_matrix::Matrix::value_array_type & y)
{
    ellpack_spmv(A.rows, A.rowLength, A.column_index.data(),
                 A.value.data(), x.data(), y.data());
}

ellpack_matrix::Matrix::value_array_type operator*(
    Matrix const & A,
    ellpack_matrix::Matrix::value_array_type const & x)
{
    if (A.columns != x.size()) {
        throw std::invalid_argument(
            "Size mismatch: "s +
            "A.size()="s + (
                std::to_string(A.rows) + "x"s + std::to_string(A.columns)) + ", " +
            "x.size()=" + std::to_string(x.size()));
    }

    ellpack_matrix::Matrix::value_array_type y(A.rows, 0.0);
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
