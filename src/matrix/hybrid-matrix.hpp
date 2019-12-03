#ifndef HYBRID_MATRIX_HPP
#define HYBRID_MATRIX_HPP

#include "matrix/coo-matrix.hpp"
#include "matrix/ellpack-matrix.hpp"
#include "util/aligned-allocator.hpp"

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace matrix_market { class Matrix; }

namespace hybrid_matrix
{

typedef int32_t size_type;
typedef int32_t index_type;
typedef double value_type;
typedef std::vector<size_type, aligned_allocator<index_type, 4096>> size_array_type;
typedef std::vector<index_type, aligned_allocator<index_type, 4096>> index_array_type;
typedef std::vector<value_type, aligned_allocator<value_type, 4096>> value_array_type;

struct Matrix
{
public:
    Matrix();
    Matrix(index_type rows,
           index_type columns,
           size_type num_entries,
           index_type ellpack_row_length,
           index_array_type const & ellpack_column_index,
           value_array_type const & ellpack_value,
           bool ellpack_skip_padding,
           size_type num_coo_entries,
           index_array_type const & coo_row_index,
           index_array_type const & coo_column_index,
           value_array_type const & coo_value);

    Matrix(Matrix const & m) = delete;
    Matrix & operator=(Matrix const & m) = delete;
    Matrix(Matrix && m) = default;
    Matrix & operator=(Matrix && m) = default;

    std::size_t size() const;
    std::size_t value_size() const;
    std::size_t index_size() const;
    size_type num_padding_entries() const;
    std::size_t value_padding_size() const;
    std::size_t index_padding_size() const;

    index_type spmv_rows_per_thread(int thread, int num_threads) const;
    size_type spmv_nonzeros_per_thread(int thread, int num_threads) const;

    std::vector<std::pair<uintptr_t, int>> spmv_memory_reference_string(
        value_array_type const & x,
        value_array_type const & y,
        value_array_type const & workspace,
        int thread,
        int num_threads,
        int const * numa_domains,
        int page_size) const;

public:
    index_type rows;
    index_type columns;
    size_type num_entries;

    size_type num_ellpack_entries;
    index_type ellpack_row_length;
    index_array_type ellpack_column_index;
    value_array_type ellpack_value;
    bool ellpack_skip_padding;

    size_type num_coo_entries;
    index_array_type coo_row_index;
    index_array_type coo_column_index;
    value_array_type coo_value;
};

bool operator==(Matrix const & a, Matrix const & b);
std::ostream & operator<<(std::ostream & o, Matrix const & x);

Matrix from_matrix_market_default(
    matrix_market::Matrix const & m);

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    bool skip_padding = false);

value_array_type operator*(
    Matrix const & A,
    value_array_type const & x);

void spmv(
    int num_threads,
    Matrix const & A,
    value_array_type const & x,
    value_array_type & y,
    value_array_type & workspace,
    index_type chunk_size = 0);

index_type spmv_rows_per_thread(
    Matrix const & A,
    int thread,
    int num_threads);

size_type spmv_nonzeros_per_thread(
    Matrix const & A,
    int thread,
    int num_threads);

}

#endif
