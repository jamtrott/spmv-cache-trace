#include "coordinate-matrix.hpp"
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

namespace coo_matrix
{

std::string to_string(Order o)
{
    switch (o) {
    case Order::general: return "general";
    case Order::column_major: return "column major";
    case Order::row_major: return "row major";
    default:
        throw std::invalid_argument("Unknown matrix order");
    }
}

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , numEntries(0)
    , order(Order::general)
    , row_index()
    , column_index()
    , value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type numEntries,
    Order order,
    index_array_type const & row_index,
    index_array_type const & column_index,
    value_array_type const & value)
    : rows(rows)
    , columns(columns)
    , numEntries(numEntries)
    , order(order)
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

std::vector<uintptr_t> Matrix::spmv_memory_reference_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_line_size) const
{
    auto w = std::vector<uintptr_t>(5 * numEntries);
    for (size_type k = 0, l = 0; k < numEntries; ++k, l += 5) {
        index_type i = row_index[k];
        index_type j = column_index[k];
        w[l] = uintptr_t(&row_index[k]) / cache_line_size;
        w[l+1] = uintptr_t(&column_index[k]) / cache_line_size;
        w[l+2] = uintptr_t(&value[k]) / cache_line_size;
        w[l+3] = uintptr_t(&x[j]) / cache_line_size;
        w[l+4] = uintptr_t(&y[i]) / cache_line_size;
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.numEntries == b.numEntries &&
        a.order == b.order &&
        a.row_index == b.row_index &&
        a.column_index == b.column_index &&
        a.value == b.value;
}

std::ostream & operator<<(std::ostream & o, Order order)
{
    switch (order) {
    case Order::general: return o << "general";
    case Order::column_major: return o << "column-major";
    case Order::row_major: return o << "row-major";
    default:
      throw std::invalid_argument("Invalid matrix order");
    }
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
             << x.numEntries << ' '
             << x.order << ' '
             << x.row_index << ' '
             << x.column_index << ' '
             << x.value;
}

Matrix from_matrix_market_general(
    matrix_market::Matrix const & m)
{
    return from_matrix_market(m, Order::general);
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    Order o)
{
    if (m.header.format != matrix_market::Format::coordinate)
        throw std::invalid_argument("Expected matrix in coordinate format");

    auto const & size = m.size;
    auto entries = m.entries;
    if (o == Order::column_major) {
        std::sort(
            std::begin(entries), std::end(entries),
            [] (auto const & a, auto const & b)
            { return std::tie(a.j, a.i) < std::tie(b.j, b.i); });
    } else if (o == Order::row_major) {
        std::sort(
            std::begin(entries), std::end(entries),
            [] (auto const & a, auto const & b)
            { return std::tie(a.i, a.j) < std::tie(b.i, b.j); });
    }

    index_array_type rows(entries.size());
    index_array_type columns(entries.size());
    value_array_type values(entries.size());
    for (size_type k = 0u; k < (size_type) entries.size(); ++k) {
        rows[k] = entries[k].i - 1;
        columns[k] = entries[k].j - 1;
        values[k] = entries[k].a;
    }

    return Matrix{
        size.rows, size.columns, size.numEntries,
        o, rows, columns, values};
}

namespace
{

void coo_spmv(
    size_type const numEntries,
    index_type const * row_index,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y)
{
    index_type i, j;
    size_type k;
    for (k = 0; k < numEntries; ++k) {
        i = row_index[k];
        j = column_index[k];
        y[i] += value[k] * x[j];
    }
}

}

void spmv(
    coo_matrix::Matrix const & A,
    coo_matrix::value_array_type const & x,
    coo_matrix::value_array_type & y)
{
    coo_spmv(A.numEntries, A.row_index.data(), A.column_index.data(),
             A.value.data(), x.data(), y.data());
}

coo_matrix::value_array_type operator*(
    coo_matrix::Matrix const & A,
    coo_matrix::value_array_type const & x)
{
    if (A.columns != (index_type) x.size()) {
        throw std::invalid_argument(
            "Size mismatch: "s +
            "A.size()="s + (
                std::to_string(A.rows) + "x"s + std::to_string(A.columns)) + ", " +
            "x.size()=" + std::to_string(x.size()));
    }

    coo_matrix::value_array_type y(A.rows, 0.0);
    coo_spmv(
        A.numEntries, A.row_index.data(), A.column_index.data(),
        A.value.data(), x.data(), y.data());
    return y;
}

}
