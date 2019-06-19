#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "coordinate-matrix.hpp"
#include "csr-matrix.hpp"
#include "ellpack-matrix.hpp"
#include "source-vector-only-matrix.hpp"
#include "util/aligned-allocator.hpp"

#include <functional>
#include <iosfwd>
#include <string>

namespace linsparse
{

enum class MatrixFormat
{
    none,
    coo,
    csr,
    csr_avx128,
    csr_avx256,
    csr_unroll2,
    csr_unroll2_avx128,
    csr_unroll2_avx256,
    csr_unroll4,
    csr_unroll4_avx128,
    csr_unroll4_avx256,
    csr_regular_traffic,
    csr_irregular_traffic,
    ellpack,
    source_vector_only
};

std::string matrix_format_name(
    MatrixFormat const & format);

MatrixFormat find_matrix_format(
    std::string const & name);

void list_matrix_formats(
    std::ostream & o);

class Matrix;
class Vector;

class Matrix
{
public:
    Matrix();
    Matrix(MatrixFormat, coo_matrix::Matrix &&);
    Matrix(MatrixFormat, csr_matrix::Matrix &&);
    Matrix(MatrixFormat, ellpack_matrix::Matrix &&);
    Matrix(MatrixFormat, source_vector_only_matrix::Matrix &&);

    MatrixFormat format() const;
    unsigned int rows() const;
    unsigned int columns() const;
    unsigned int nonzeros() const;
    std::size_t size() const;
    void synchronize();

    std::vector<uintptr_t> spmv_memory_reference_reference_string(
        Vector const & x,
        Vector const & y,
        unsigned int thread,
        unsigned int num_threads,
        unsigned int cache_line_size) const;

private:
    MatrixFormat _format;
    coo_matrix::Matrix _coo_matrix;
    csr_matrix::Matrix _csr_matrix;
    ellpack_matrix::Matrix _ellpack_matrix;
    source_vector_only_matrix::Matrix _source_vector_only_matrix;

private:
    friend void spmv(
        Matrix const & m,
        Vector const & x,
        Vector & y);

    friend int spmv_rows_per_thread(
        Matrix const & m,
        int thread,
        int num_threads);

    friend int spmv_nonzeros_per_thread(
        Matrix const & m,
        int thread,
        int num_threads);
};

Matrix from_matrix_market(
    matrix_market::Matrix const & market_matrix,
    MatrixFormat const & matrix_format,
    std::ostream & o,
    bool verbose = false);

class Vector
{
public:
    Vector();
    Vector(
        MatrixFormat const & format,
        std::vector<double> const & v);

    std::vector<double, aligned_allocator<double, 64u>> & vector();
    std::vector<double, aligned_allocator<double, 64u>> const & vector() const;

private:
    MatrixFormat _format;
    std::vector<double, aligned_allocator<double, 64u>> _vector;

private:
    friend void spmv(
        Matrix const & m,
        Vector const & x,
        Vector & y);
};

Vector make_vector(
    MatrixFormat const & format,
    std::vector<double> const & v);

void spmv(
    Matrix const & m,
    Vector const & x,
    Vector & y);

int spmv_rows_per_thread(
    Matrix const & m,
    int thread,
    int num_threads);

int spmv_nonzeros_per_thread(
    Matrix const & m,
    int thread,
    int num_threads);

}

#endif
