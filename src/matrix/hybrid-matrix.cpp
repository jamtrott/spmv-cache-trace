#include "hybrid-matrix.hpp"
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

namespace hybrid_matrix
{

Matrix::Matrix()
    : rows(0)
    , columns(0)
    , num_entries(0)
    , ellpack_row_length(0)
    , num_ellpack_entries(0)
    , ellpack_column_index()
    , ellpack_value()
    , ellpack_skip_padding(false)
    , num_coo_entries(0)
    , coo_row_index()
    , coo_column_index()
    , coo_value()
{
}

Matrix::Matrix(
    index_type rows,
    index_type columns,
    size_type num_entries,
    index_type ellpack_row_length,
    size_type num_ellpack_entries,
    index_array_type const & ellpack_column_index,
    value_array_type const & ellpack_value,
    bool ellpack_skip_padding,
    size_type num_coo_entries,
    index_array_type const & coo_row_index,
    index_array_type const & coo_column_index,
    value_array_type const & coo_value)
    : rows(rows)
    , columns(columns)
    , num_entries(num_entries)
    , ellpack_row_length(ellpack_row_length)
    , num_ellpack_entries(num_ellpack_entries)
    , ellpack_column_index(ellpack_column_index)
    , ellpack_value(ellpack_value)
    , ellpack_skip_padding(ellpack_skip_padding)
    , num_coo_entries(num_coo_entries)
    , coo_row_index(coo_row_index)
    , coo_column_index(coo_column_index)
    , coo_value(coo_value)
{
}

std::size_t Matrix::size() const
{
    return value_size() + index_size();
}

std::size_t Matrix::value_size() const
{
    return
        (sizeof(decltype(ellpack_value)::value_type) *
         ellpack_value.size()) +
        (sizeof(decltype(ellpack_value)::value_type) * coo_value.size());
}

std::size_t Matrix::index_size() const
{
    return
        (sizeof(decltype(ellpack_column_index)::value_type) *
         ellpack_column_index.size()) +
        (sizeof(decltype(coo_column_index)::value_type) * coo_column_index.size());
}

size_type Matrix::num_padding_entries() const
{
    return (ellpack_value.size() + coo_value.size()) - num_entries;
}

std::size_t Matrix::value_padding_size() const
{
    return sizeof(decltype(ellpack_value)::value_type) * num_padding_entries();
}

std::size_t Matrix::index_padding_size() const
{
    return sizeof(decltype(ellpack_column_index)::value_type) * num_padding_entries();
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
    size_type start_nonzero = start_row * ellpack_row_length;
    size_type end_nonzero = end_row * ellpack_row_length;
    size_type nonzeros = end_nonzero - start_nonzero;
    return nonzeros;
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
    index_type rows_per_thread = (rows + num_threads - 1) / num_threads;
    index_type start_row = std::min(rows, thread * rows_per_thread);
    index_type end_row = std::min(rows, (thread + 1) * rows_per_thread);
    index_type rows = end_row - start_row;
    size_type start_nonzero = start_row * ellpack_row_length;
    size_type end_nonzero = end_row * ellpack_row_length;
    size_type nonzeros = end_nonzero - start_nonzero;

    size_type num_references = 3 * nonzeros + 1 * rows;
    std::vector<std::pair<uintptr_t, int>> w(
        num_references, std::make_pair(0,0));
    size_type l = 0;
    for (index_type i = start_row; i < end_row; ++i) {
        for (size_type k = i * ellpack_row_length; k < (i+1)*ellpack_row_length; ++k) {
            index_type j = ellpack_column_index[k];
            w[l++] = std::make_pair(
                uintptr_t(&ellpack_column_index[k]),
                numa_domains[thread]);
            w[l++] = std::make_pair(
                uintptr_t(&ellpack_value[k]),
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
        a.ellpack_row_length == b.ellpack_row_length &&
        a.num_ellpack_entries == b.num_ellpack_entries &&
        std::equal(
            std::begin(a.ellpack_column_index),
            std::end(a.ellpack_column_index),
            std::begin(b.ellpack_column_index)) &&
        std::equal(
            std::begin(a.ellpack_value),
            std::end(a.ellpack_value),
            std::begin(b.ellpack_value)) &&
        std::equal(
            std::begin(a.coo_row_index),
            std::end(a.coo_row_index),
            std::begin(b.coo_row_index)) &&
        std::equal(
            std::begin(a.coo_column_index),
            std::end(a.coo_column_index),
            std::begin(b.coo_column_index)) &&
        std::equal(
            std::begin(a.coo_value),
            std::end(a.coo_value),
            std::begin(b.coo_value));
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
             << x.ellpack_row_length << ' '
             << x.num_ellpack_entries << ' '
             << x.ellpack_column_index << ' '
             << x.ellpack_value << ' '
             << x.num_coo_entries << ' '
             << x.coo_row_index << ' '
             << x.coo_column_index << ' '
             << x.coo_value;
}

Matrix from_matrix_market_default(
    matrix_market::Matrix const & m,
    std::ostream & o,
    bool verbose)
{
    return from_matrix_market(m, false, o, verbose);
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    bool ellpack_skip_padding,
    std::ostream & o,
    bool verbose)
{
    if (verbose) {
        o << "Converting matrix to hybrid format" << std::endl;
    }

    if (m.format() != matrix_market::Format::coordinate)
        throw matrix::matrix_error("Expected matrix in coordinate format");

    /* Compute the histogram of row lengths. */
    std::vector<index_type> row_lengths = m.row_lengths();
    index_type max_row_length = *std::max_element(std::cbegin(row_lengths), std::cend(row_lengths));
    std::vector<index_type> num_rows_per_row_length(max_row_length+1, 0);
    for (index_type i = 0; i < m.rows(); i++) {
        num_rows_per_row_length[row_lengths[i]]++;
    }

    /* Choose the row length for ELLPACK as the 2/3 median. */
    index_type median_row_length = 0;
    index_type num_rows_less_than_median = 0;
    while (num_rows_less_than_median < (2*m.rows()) / 3) {
        num_rows_less_than_median += num_rows_per_row_length[median_row_length];
        median_row_length++;
    }
    median_row_length = (median_row_length == 0) ? 0 : median_row_length-1;

    // Compute the row length and number of entries (including padding)
    index_type rows = m.rows();
    index_type ellpack_row_length = median_row_length;
    size_type num_ellpack_entries;
    if (__builtin_mul_overflow(rows, ellpack_row_length, &num_ellpack_entries)) {
        throw matrix::matrix_error(
            "Failed to convert to HYBRID: "
            "Integer overflow when computing number of non-zeros");
    }
    num_ellpack_entries = rows * ellpack_row_length;

    /* The remaining entries are stored in COO format. */
    index_type num_coo_entries = 0;
    for (index_type l = ellpack_row_length+1; l <= max_row_length; l++) {
        num_coo_entries += num_rows_per_row_length[l] * (l - ellpack_row_length);
    }

    // Sort the matrix entries
    matrix_market::Matrix m_sorted = sort_matrix_row_major(m);
    auto row_indices = m_sorted.row_indices();
    auto column_indices = m_sorted.column_indices();
    auto values = m_sorted.values_real();

    // Insert the values and column indices with the required padding
    index_array_type ellpack_columns(num_ellpack_entries, 0);
    value_array_type ellpack_values(num_ellpack_entries, 0.0);
    index_array_type coo_rows(num_coo_entries, 0);
    index_array_type coo_columns(num_coo_entries, 0);
    value_array_type coo_values(num_coo_entries, 0.0);
    size_type k = 0;
    num_ellpack_entries = 0;
    num_coo_entries = 0;
    for (index_type r = 0; r < m.rows(); ++r) {
        if (row_lengths[r] < ellpack_row_length) {
            for (index_type j = 0; j < row_lengths[r]; j++) {
                ellpack_columns[num_ellpack_entries] = column_indices[k] - 1;
                ellpack_values[num_ellpack_entries] = values[k];
                num_ellpack_entries++;
                k++;
            }

            for (index_type j = row_lengths[r]; j < ellpack_row_length; j++) {
                ellpack_columns[num_ellpack_entries] = ellpack_skip_padding
                    ? std::numeric_limits<index_type>::max()
                    : (k>0 ? column_indices[k-1] - 1 : 0);
                ellpack_values[num_ellpack_entries] = 0.0;
                num_ellpack_entries++;
            }
        } else {
            for (index_type j = 0; j < ellpack_row_length; j++) {
                ellpack_columns[num_ellpack_entries] = column_indices[k] - 1;
                ellpack_values[num_ellpack_entries] = values[k];
                num_ellpack_entries++;
                k++;
            }            

            for (index_type j = ellpack_row_length; j < row_lengths[r]; j++) {
                coo_rows[num_coo_entries] = row_indices[k] - 1;
                coo_columns[num_coo_entries] = column_indices[k] - 1;
                coo_values[num_coo_entries] = values[k];
                num_coo_entries++;
                k++;
            }
        }
    }

    return Matrix(
        m.rows(), m.columns(), m.num_entries(),
        ellpack_row_length, num_ellpack_entries,
        ellpack_columns, ellpack_values, ellpack_skip_padding,
        num_coo_entries, coo_rows, coo_columns, coo_values);
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

/*
 * COO SpMV.
 */
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

/*
 * Hybrid SpMV.
 */
void spmv(
    int num_threads,
    hybrid_matrix::Matrix const & A,
    hybrid_matrix::value_array_type const & x,
    hybrid_matrix::value_array_type & y,
    hybrid_matrix::value_array_type & workspace,
    hybrid_matrix::index_type chunk_size)
{
    if (chunk_size <= 0) {
        chunk_size = (A.rows + num_threads - 1) / num_threads;
    }

    if (!A.ellpack_skip_padding) {
        ellpack_spmv(
            A.rows, A.ellpack_row_length, A.ellpack_column_index.data(),
            A.ellpack_value.data(), x.data(), y.data(), chunk_size);
    } else {
        ellpack_spmv_skip_padding(
            A.rows, A.ellpack_row_length, A.ellpack_column_index.data(),
            A.ellpack_value.data(), x.data(), y.data(), chunk_size);
    }

    coo_spmv(num_threads,
             A.rows,
             A.num_coo_entries,
             A.coo_row_index.data(),
             A.coo_column_index.data(),
             A.coo_value.data(),
             x.data(),
             y.data(),
             workspace.data(),
             chunk_size);
}

hybrid_matrix::value_array_type operator*(
    hybrid_matrix::Matrix const & A,
    hybrid_matrix::value_array_type const & x)
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

    hybrid_matrix::value_array_type y(A.rows, 0.0);
    hybrid_matrix::value_array_type workspace(num_threads*A.rows, 0.0);
    spmv(num_threads, A, x, y, workspace, 0);
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
