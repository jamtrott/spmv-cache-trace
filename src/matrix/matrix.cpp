#include "matrix.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <map>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

namespace linsparse
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
#ifdef HAVE_CUDA
    {"cuda_coo", MatrixFormat::cuda_coo},
    {"cuda_csr", MatrixFormat::cuda_csr},
    {"cuda_ellpack", MatrixFormat::cuda_ellpack},
    {"cuda_sliced_ellpack", MatrixFormat::cuda_sliced_ellpack},
#endif
};

std::string matrix_format_name(
    MatrixFormat const & format)
{
    auto it = std::cbegin(matrix_formats);
    for (; it != std::cend(matrix_formats); ++it) {
        if (format == it->second)
            return it->first;
    }

    throw std::invalid_argument("Unknown matrix format");
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

#ifdef HAVE_CUDA
Matrix::Matrix(MatrixFormat format, cuda_coo_matrix::Matrix && m)
    : _format(format)
    , _cuda_coo_matrix(std::move(std::move(m)))
{
}

Matrix::Matrix(MatrixFormat format, cuda_csr_matrix::Matrix && m)
    : _format(format)
    , _cuda_csr_matrix(std::move(m))
{
}

Matrix::Matrix(MatrixFormat format, cuda_ellpack_matrix::Matrix && m)
    : _format(format)
    , _cuda_ellpack_matrix(std::move(m))
{
}

Matrix::Matrix(MatrixFormat format, cuda_sliced_ellpack_matrix::Matrix && m)
    : _format(format)
    , _cuda_sliced_ellpack_matrix(std::move(m))
{
}
#endif

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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo: return _cuda_coo_matrix.rows;
    case MatrixFormat::cuda_csr: return _cuda_csr_matrix.rows;
    case MatrixFormat::cuda_ellpack: return _cuda_ellpack_matrix.rows;
    case MatrixFormat::cuda_sliced_ellpack: return _cuda_sliced_ellpack_matrix.rows;
#endif
    default:
        throw std::logic_error("Not implemented");
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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo: return _cuda_coo_matrix.columns;
    case MatrixFormat::cuda_csr: return _cuda_csr_matrix.columns;
    case MatrixFormat::cuda_ellpack: return _cuda_ellpack_matrix.columns;
    case MatrixFormat::cuda_sliced_ellpack: return _cuda_sliced_ellpack_matrix.columns;
#endif
    default:
        throw std::logic_error("Not implemented");
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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo: return _cuda_coo_matrix.numEntries;
    case MatrixFormat::cuda_csr: return _cuda_csr_matrix.numEntries;
    case MatrixFormat::cuda_ellpack: return _cuda_ellpack_matrix.numEntries;
    case MatrixFormat::cuda_sliced_ellpack: return _cuda_sliced_ellpack_matrix.numEntries;
#endif
    default:
        throw std::logic_error("Not implemented");
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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo: return _cuda_coo_matrix.size();
    case MatrixFormat::cuda_csr: return _cuda_csr_matrix.size();
    case MatrixFormat::cuda_ellpack: return _cuda_ellpack_matrix.size();
    case MatrixFormat::cuda_sliced_ellpack: return _cuda_sliced_ellpack_matrix.size();
#endif
    default:
        throw std::logic_error("Not implemented");
    }
}

void Matrix::synchronize()
{
#ifdef HAVE_CUDA
    cudaDeviceSynchronize();
#endif
}

std::vector<uintptr_t> Matrix::spmv_page_reference_string(
    Vector const & x,
    Vector const & y,
    unsigned int thread,
    unsigned int num_threads,
    unsigned int cache_line_size) const
{
    switch (_format) {
    case MatrixFormat::coo:
        return _coo_matrix.spmv_page_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
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
        return _csr_matrix.spmv_page_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::ellpack:
        return _ellpack_matrix.spmv_page_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::source_vector_only:
        return _source_vector_only_matrix.spmv_page_reference_string(
            x.vector(), y.vector(), thread, num_threads, cache_line_size);
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo:
        return _cuda_coo_matrix.spmv_page_reference_string(
            x.cuda_vector(), y.cuda_vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::cuda_csr:
        return _cuda_csr_matrix.spmv_page_reference_string(
            x.cuda_vector(), y.cuda_vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::cuda_ellpack:
        return _cuda_ellpack_matrix.spmv_page_reference_string(
            x.cuda_vector(), y.cuda_vector(), thread, num_threads, cache_line_size);
    case MatrixFormat::cuda_sliced_ellpack:
        return _cuda_sliced_ellpack_matrix.spmv_page_reference_string(
            x.cuda_vector(), y.cuda_vector(), thread, num_threads, cache_line_size);
#endif
    default:
        throw std::logic_error("Not implemented");
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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo:
        return Matrix(matrix_format, cuda_coo_matrix::from_matrix_market(m));
    case MatrixFormat::cuda_csr:
        return Matrix(matrix_format, cuda_csr_matrix::from_matrix_market(m));
    case MatrixFormat::cuda_ellpack:
        return Matrix(matrix_format, cuda_ellpack_matrix::from_matrix_market(m));
    case MatrixFormat::cuda_sliced_ellpack:
        return Matrix(matrix_format, cuda_sliced_ellpack_matrix::from_matrix_market_default(m));
#endif
    default:
        throw std::logic_error("Not implemented");
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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo:
    case MatrixFormat::cuda_csr:
    case MatrixFormat::cuda_ellpack:
    case MatrixFormat::cuda_sliced_ellpack:
        _cuda_vector = cuda::vector<double>(v);
        break;
#endif
    default:
        throw std::logic_error("Not implemented");
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

#ifdef HAVE_CUDA
cuda::vector<double> & Vector::cuda_vector()
{
    return _cuda_vector;
}

cuda::vector<double> const & Vector::cuda_vector() const
{
    return _cuda_vector;
}
#endif

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
#ifdef HAVE_CUDA
    case MatrixFormat::cuda_coo:
        cuda_coo_matrix::spmv(m._cuda_coo_matrix, x._cuda_vector, y._cuda_vector);
        cudaDeviceSynchronize();
        break;
    case MatrixFormat::cuda_csr:
        cuda_csr_matrix::spmv(m._cuda_csr_matrix, x._cuda_vector, y._cuda_vector);
        cudaDeviceSynchronize();
        break;
    case MatrixFormat::cuda_ellpack:
        cuda_ellpack_matrix::spmv(m._cuda_ellpack_matrix, x._cuda_vector, y._cuda_vector);
        cudaDeviceSynchronize();
        break;
    case MatrixFormat::cuda_sliced_ellpack:
        cuda_sliced_ellpack_matrix::spmv(m._cuda_sliced_ellpack_matrix, x._cuda_vector, y._cuda_vector);
        cudaDeviceSynchronize();
        break;
#endif
    default:
        throw std::logic_error("Not implemented");
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
        throw std::logic_error("Not implemented");
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
        throw std::logic_error("Not implemented");
    }
}

}
