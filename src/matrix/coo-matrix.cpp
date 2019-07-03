#include "coo-matrix.hpp"
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

namespace coo_matrix
{

std::string to_string(Order o)
{
    switch (o) {
    case Order::general: return "general";
    case Order::column_major: return "column major";
    case Order::row_major: return "row major";
    default:
        throw matrix::matrix_error("Unknown matrix order");
    }
}

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , num_entries(0)
    , order(Order::general)
    , row_index()
    , column_index()
    , value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    Order order,
    index_array_type const & row_index,
    index_array_type const & column_index,
    value_array_type const & value)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
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

int thread_of_index(
    index_type index,
    index_type num_indices,
    int num_threads)
{
    index_type indices_per_thread = (num_indices + num_threads - 1) / num_threads;
    for (int thread = 0 ; thread < num_threads; thread++) {
        index_type start_indices = std::min(num_indices, thread * indices_per_thread);
        index_type end_indices = std::min(num_indices, (thread + 1) * indices_per_thread);
        if (index >= start_indices && index <= end_indices)
            return thread;
    }
    return num_threads-1;
}

std::vector<std::pair<uintptr_t, int>> Matrix::spmv_memory_reference_string(
    value_array_type const & x,
    value_array_type const & y,
    int thread,
    int num_threads,
    int const * numa_domains) const
{
    index_type entries_per_thread = (num_entries + num_threads - 1) / num_threads;
    index_type thread_start_entry = std::min(num_entries, thread * entries_per_thread);
    index_type thread_end_entry = std::min(num_entries, (thread + 1) * entries_per_thread);
    index_type thread_num_entries = thread_end_entry - thread_start_entry;

    auto w = std::vector<std::pair<uintptr_t, int>>(5 * thread_num_entries);
    for (size_type k = thread_start_entry, l = 0; k < thread_end_entry; ++k, l += 5) {
        index_type i = row_index[k];
        index_type j = column_index[k];
        w[l] = std::make_pair(
            uintptr_t(&row_index[k]),
            numa_domains[thread]);
        w[l+1] = std::make_pair(
            uintptr_t(&column_index[k]),
            numa_domains[thread]);
        w[l+2] = std::make_pair(
            uintptr_t(&value[k]),
            numa_domains[thread]);
        w[l+3] = std::make_pair(
            uintptr_t(&x[j]),
            numa_domains[thread_of_index(j, columns, num_threads)]);
        w[l+4] = std::make_pair(
            uintptr_t(&y[i]),
            numa_domains[thread_of_index(i, rows, num_threads)]);
    }
    return w;
}

bool operator==(Matrix const & a, Matrix const & b)
{
    return a.rows == b.rows &&
        a.columns == b.columns &&
        a.num_entries == b.num_entries &&
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
      throw matrix::matrix_error("Invalid matrix order");
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
             << x.num_entries << ' '
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
        throw matrix::matrix_error("Expected matrix in coordinate format");

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
        size.rows, size.columns, size.num_entries,
        o, rows, columns, values};
}

namespace
{

void coo_spmv(
    size_type const num_entries,
    index_type const * row_index,
    index_type const * column_index,
    value_type const * value,
    value_type const * x,
    value_type * y)
{
    index_type i, j;
    size_type k;
    #pragma omp parallel for
    for (k = 0; k < num_entries; ++k) {
        i = row_index[k];
        j = column_index[k];
        #pragma omp atomic
        y[i] += value[k] * x[j];
    }
}

}

void spmv(
    coo_matrix::Matrix const & A,
    coo_matrix::value_array_type const & x,
    coo_matrix::value_array_type & y)
{
    coo_spmv(A.num_entries, A.row_index.data(), A.column_index.data(),
             A.value.data(), x.data(), y.data());
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

    coo_matrix::value_array_type y(A.rows, 0.0);
    coo_spmv(
        A.num_entries, A.row_index.data(), A.column_index.data(),
        A.value.data(), x.data(), y.data());
    return y;
}

}
