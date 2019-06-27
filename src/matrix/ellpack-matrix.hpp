#ifndef ELLPACK_MATRIX_HPP
#define ELLPACK_MATRIX_HPP

#include "util/aligned-allocator.hpp"

#include <iosfwd>
#include <valarray>
#include <vector>

namespace matrix_market { class Matrix; }

namespace ellpack_matrix
{

typedef int64_t size_type;
typedef int32_t index_type;
typedef double value_type;
typedef std::vector<size_type, aligned_allocator<index_type, 64>> size_array_type;
typedef std::vector<index_type, aligned_allocator<index_type, 64>> index_array_type;
typedef std::vector<value_type, aligned_allocator<value_type, 64>> value_array_type;

struct Matrix
{
public:
    Matrix();
    Matrix(index_type rows,
           index_type columns,
           size_type num_entries,
           index_type row_length,
           index_array_type const & column_index,
           value_array_type const & value,
           bool skip_padding = false);

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

    std::vector<std::pair<uintptr_t, int>> spmv_memory_reference_string(
        value_array_type const & x,
        value_array_type const & y,
        unsigned int thread,
        unsigned int num_threads,
        unsigned int cache_line_size) const;

public:
    index_type rows;
    index_type columns;
    size_type num_entries;
    index_type row_length;
    index_array_type column_index;
    value_array_type value;
    bool skip_padding;
};

bool operator==(Matrix const & a, Matrix const & b);
std::ostream & operator<<(std::ostream & o, Matrix const & x);

Matrix from_matrix_market_default(
    matrix_market::Matrix const & m);

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    bool skip_padding = false);

void spmv(
    Matrix const & A,
    value_array_type const & x,
    value_array_type & y);

std::valarray<value_type> operator*(
    Matrix const & A,
    std::valarray<value_type> const & x);

value_array_type operator*(
    Matrix const & A,
    value_array_type const & x);

}

#endif
