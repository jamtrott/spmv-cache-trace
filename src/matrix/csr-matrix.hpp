#ifndef CSR_MATRIX_HPP
#define CSR_MATRIX_HPP

#include "util/aligned-allocator.hpp"

#include <iosfwd>
#include <vector>

namespace matrix_market { class Matrix; }

namespace csr_matrix
{

struct Matrix
{
public:

    typedef int index_type;
    typedef double value_type;
    typedef std::vector<index_type, aligned_allocator<index_type, 64>> index_array_type;
    typedef std::vector<value_type, aligned_allocator<value_type, 64>> value_array_type;

public:
    Matrix();
    Matrix(int rows,
           int columns,
           int numEntries,
           int row_alignment,
           index_array_type const row_ptr,
           index_array_type const column_index,
           value_array_type const value);

    Matrix(Matrix const & m) = delete;
    Matrix & operator=(Matrix const & m) = delete;
    Matrix(Matrix && m) = default;
    Matrix & operator=(Matrix && m) = default;

    std::size_t size() const;
    std::size_t value_size() const;
    std::size_t index_size() const;
    int num_padding_entries() const;
    std::size_t value_padding_size() const;
    std::size_t index_padding_size() const;

    int spmv_rows_per_thread(int thread, int num_threads) const;
    int spmv_nonzeros_per_thread(int thread, int num_threads) const;

    std::vector<uintptr_t> spmv_memory_reference_reference_string(
        value_array_type const & x,
        value_array_type const & y,
        int thread,
        int num_threads,
        int cache_line_size) const;

public:
    int const rows;
    int const columns;
    int const numEntries;
    int row_alignment;
    index_array_type const row_ptr;
    index_array_type const column_index;
    value_array_type const value;
};

bool operator==(Matrix const & a, Matrix const & b);
std::ostream & operator<<(std::ostream & o, Matrix const & x);

Matrix from_matrix_market(
    matrix_market::Matrix const & m);

Matrix from_matrix_market_row_aligned(
    matrix_market::Matrix const & m,
    int row_alignment);

Matrix::value_array_type operator*(
    Matrix const & A,
    Matrix::value_array_type const & x);

void spmv(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y,
    int chunk_size = 0);

void spmv_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll2(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll2_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll2_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll4(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll4_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_unroll4_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_regular_traffic(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

void spmv_irregular_traffic(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y);

int spmv_rows_per_thread(
    Matrix const & A,
    int thread,
    int num_threads);

int spmv_nonzeros_per_thread(
    Matrix const & A,
    int thread,
    int num_threads);

}

#endif
