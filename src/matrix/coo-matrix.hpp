#ifndef COORDINATE_MATRIX_HPP
#define COORDINATE_MATRIX_HPP

#include "util/aligned-allocator.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace matrix_market { class Matrix; }

namespace coo_matrix
{

typedef int32_t size_type;
typedef int32_t index_type;
typedef double value_type;
typedef std::vector<index_type, aligned_allocator<index_type, 4096>> index_array_type;
typedef std::vector<value_type, aligned_allocator<value_type, 4096>> value_array_type;

class Matrix
{
public:
    Matrix();
    Matrix(
        index_type rows,
        index_type columns,
        size_type num_entries,
        index_array_type const & row_index,
        index_array_type const & column_index,
        value_array_type const & value);

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
        value_array_type const & workspace,
        int thread,
        int num_threads,
        int const * numa_domains,
        int page_size) const;

    std::vector<std::pair<uintptr_t, int>> spmv_atomic_memory_reference_string(
        value_array_type const & x,
        value_array_type const & y,
        int thread,
        int num_threads,
        int const * numa_domains,
        int page_size) const;

public:
    index_type rows;
    index_type columns;
    size_type num_entries;
    index_array_type row_index;
    index_array_type column_index;
    value_array_type value;
};

bool operator==(Matrix const & a, Matrix const & b);
std::ostream & operator<<(std::ostream & o, Matrix const & x);

Matrix from_matrix_market(
    matrix_market::Matrix const & m);

void spmv(
    int num_threads,
    Matrix const & A,
    value_array_type const & x,
    value_array_type & y,
    value_array_type & workspace,
    index_type chunk_size = 0);

void spmv_atomic(
    int num_threads,
    Matrix const & A,
    value_array_type const & x,
    value_array_type & y,
    index_type chunk_size = 0);

value_array_type operator*(
    Matrix const & A,
    value_array_type const & x);

}

#endif
