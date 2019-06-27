#include "matrix.hpp"
#include "matrix-error.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <map>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

namespace matrix
{

std::map<std::string, MatrixFormat> const matrix_formats{
    {"coo", MatrixFormat::coo},
    {"csr", MatrixFormat::csr},
    {"csr_unroll2", MatrixFormat::csr_unroll2},
    {"csr_unroll4", MatrixFormat::csr_unroll4},
    {"csr_regular_traffic", MatrixFormat::csr_regular_traffic},
    {"csr_irregular_traffic", MatrixFormat::csr_irregular_traffic},
#ifdef __AVX__
    {"csr_avx128", MatrixFormat::csr_avx128},
    {"csr_unroll2_avx128", MatrixFormat::csr_unroll2_avx128},
    {"csr_unroll4_avx128", MatrixFormat::csr_unroll4_avx128},
#endif
#ifdef __AVX2__
    {"csr_avx256", MatrixFormat::csr_avx256},
    {"csr_unroll2_avx256", MatrixFormat::csr_unroll2_avx256},
    {"csr_unroll4_avx256", MatrixFormat::csr_unroll4_avx256},
#endif
    {"ellpack", MatrixFormat::ellpack},
    {"source_vector_only", MatrixFormat::source_vector_only},
};

std::string matrix_format_name(
    MatrixFormat const & format)
{
    auto it = std::cbegin(matrix_formats);
    for (; it != std::cend(matrix_formats); ++it) {
        if (format == it->second)
            return it->first;
    }

    throw matrix_error("Unknown matrix format");
}

std::ostream & operator<<(
    std::ostream & o,
    MatrixFormat const & format)
{
    return o << matrix_format_name(format);
}

MatrixFormat find_matrix_format(
    std::string const & name)
{
    auto it = matrix_formats.find(name);
    if (it == matrix_formats.end())
        return MatrixFormat::none;
    return it->second;
}

void list_matrix_formats(
    std::ostream & o)
{
    std::vector<std::string> format_names;
    std::transform(
        std::cbegin(matrix_formats),
        std::cend(matrix_formats),
        std::back_inserter(format_names),
        [] (auto const & p) { return std::string("  ") + p.first; });

    o << "Matrix formats:" << '\n';
    std::ostream_iterator<std::string> out(o, "\n");
    std::copy(std::begin(format_names), std::end(format_names), out);
}

Matrix::Matrix()
    : _format(MatrixFormat::none)
{
}

Matrix::Matrix(MatrixFormat format, coo_matrix::Matrix && m)
    : _format(format)
    , _coo_matrix(std::move(m))
{
}

Matrix::Matrix(MatrixFormat format, csr_matrix::Matrix && m)
    : _format(format)
    , _csr_matrix(std::move(m))
{
}

Matrix::Matrix(MatrixFormat format, ellpack_matrix::Matrix && m)
    : _format(format)
    , _ellpack_matrix(std::move(m))
{
}

Matrix::Matrix(MatrixFormat format, source_vector_only_matrix::Matrix && m)
    : _format(format)
    , _source_vector_only_matrix(std::move(m))
{
}

MatrixFormat Matrix::format() const
{
    return _format;
}

unsigned int Matrix::rows() const
{
    switch (_format) {
    case MatrixFormat::coo: return _coo_matrix.rows;
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return _csr_matrix.rows;
    case MatrixFormat::ellpack: return _ellpack_matrix.rows;
    case MatrixFormat::source_vector_only: return _source_vector_only_matrix.rows;
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

unsigned int Matrix::columns() const
{
    switch (_format) {
    case MatrixFormat::coo: return _coo_matrix.columns;
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return _csr_matrix.columns;
    case MatrixFormat::ellpack: return _ellpack_matrix.columns;
    case MatrixFormat::source_vector_only: return _source_vector_only_matrix.columns;
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

unsigned int Matrix::nonzeros() const
{
    switch (_format) {
    case MatrixFormat::coo: return _coo_matrix.numEntries;
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return _csr_matrix.numEntries;
    case MatrixFormat::ellpack: return _ellpack_matrix.numEntries;
    case MatrixFormat::source_vector_only: return _source_vector_only_matrix.numEntries;
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

std::size_t Matrix::size() const
{
    switch (_format) {
    case MatrixFormat::coo: return _coo_matrix.size();
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return _csr_matrix.size();
    case MatrixFormat::ellpack: return _ellpack_matrix.size();
    case MatrixFormat::source_vector_only: return _source_vector_only_matrix.size();
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

void Matrix::synchronize()
{
}

std::vector<std::pair<uintptr_t, int>> Matrix::spmv_memory_reference_string(
    Vector const & x,
    Vector const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_line_size,
    int const * numa_domains) const
{
    switch (_format) {
    case MatrixFormat::coo:
        return _coo_matrix.spmv_memory_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size,
            numa_domains);
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return _csr_matrix.spmv_memory_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size,
            numa_domains);
    case MatrixFormat::ellpack:
        return _ellpack_matrix.spmv_memory_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::source_vector_only:
        return _source_vector_only_matrix.spmv_memory_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

Matrix from_matrix_market(
    matrix_market::Matrix const & m,
    MatrixFormat const & matrix_format,
    std::ostream & o,
    bool verbose)
{
    if (verbose)
        o << "Converting matrix to " << matrix_format_name(matrix_format) << '\n';

    switch (matrix_format) {
    case MatrixFormat::coo:
        return Matrix(matrix_format, coo_matrix::from_matrix_market_general(m));
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return Matrix(matrix_format, csr_matrix::from_matrix_market(m));
    case MatrixFormat::ellpack:
        return Matrix(matrix_format, ellpack_matrix::from_matrix_market_default(m));
    case MatrixFormat::source_vector_only:
        return Matrix(matrix_format, source_vector_only_matrix::from_matrix_market_general(m));
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

Vector::Vector()
    : _format(MatrixFormat::none)
{
}

Vector::Vector(
    MatrixFormat const & format,
    std::vector<double> const & v)
    : _format(format)
{
    switch (format) {
    case MatrixFormat::coo:
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
    case MatrixFormat::ellpack:
    case MatrixFormat::source_vector_only:
        _vector = std::vector<double, aligned_allocator<double, 64u>>(
            std::cbegin(v), std::cend(v));
        break;
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

std::vector<double, aligned_allocator<double, 64u>> & Vector::vector()
{
    return _vector;
}

std::vector<double, aligned_allocator<double, 64u>> const & Vector::vector() const
{
    return _vector;
}

Vector make_vector(
    MatrixFormat const & format,
    std::vector<double> const & v)
{
    return Vector(format, v);
}

void spmv(
    Matrix const & m,
    Vector const & x,
    Vector & y)
{
    switch (m.format()) {
    case MatrixFormat::coo:
        coo_matrix::spmv(m._coo_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr:
        csr_matrix::spmv(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll2:
        csr_matrix::spmv_unroll2(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll4:
        csr_matrix::spmv_unroll4(m._csr_matrix, x._vector, y._vector);
        break;
#ifdef __AVX__
    case MatrixFormat::csr_avx128:
        csr_matrix::spmv_avx128(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll2_avx128:
        csr_matrix::spmv_unroll2_avx128(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll4_avx128:
        csr_matrix::spmv_unroll4_avx128(m._csr_matrix, x._vector, y._vector);
        break;
#endif
#ifdef __AVX2__
    case MatrixFormat::csr_avx256:
        csr_matrix::spmv_avx256(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll2_avx256:
        csr_matrix::spmv_unroll2_avx256(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_unroll4_avx256:
        csr_matrix::spmv_unroll4_avx256(m._csr_matrix, x._vector, y._vector);
        break;
#endif
    case MatrixFormat::csr_regular_traffic:
        csr_matrix::spmv_regular_traffic(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::csr_irregular_traffic:
        csr_matrix::spmv_irregular_traffic(m._csr_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::ellpack:
        ellpack_matrix::spmv(m._ellpack_matrix, x._vector, y._vector);
        break;
    case MatrixFormat::source_vector_only:
        source_vector_only_matrix::spmv(m._source_vector_only_matrix, x._vector, y._vector);
        break;
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

int spmv_rows_per_thread(
    Matrix const & m,
    int thread,
    int num_threads)
{
    switch (m.format()) {
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return csr_matrix::spmv_rows_per_thread(m._csr_matrix, thread, num_threads);
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

int spmv_nonzeros_per_thread(
    Matrix const & m,
    int thread,
    int num_threads)
{
    switch (m.format()) {
    case MatrixFormat::csr:
    case MatrixFormat::csr_avx128:
    case MatrixFormat::csr_avx256:
    case MatrixFormat::csr_unroll2:
    case MatrixFormat::csr_unroll2_avx128:
    case MatrixFormat::csr_unroll2_avx256:
    case MatrixFormat::csr_unroll4:
    case MatrixFormat::csr_unroll4_avx128:
    case MatrixFormat::csr_unroll4_avx256:
    case MatrixFormat::csr_regular_traffic:
    case MatrixFormat::csr_irregular_traffic:
        return csr_matrix::spmv_nonzeros_per_thread(m._csr_matrix, thread, num_threads);
    default:
        throw matrix_error(""s + __FUNCTION__ + ": Not implemented"s);
    }
}

}
