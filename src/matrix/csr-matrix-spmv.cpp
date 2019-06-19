#include "csr-matrix.hpp"

#include <omp.h>

#ifdef __AVX__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <cassert>
#include <vector>

using namespace csr_matrix;

inline void csr_spmv_inner_loop(
    int i,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    double z = 0.0;
    for (int k = p[i]; k < p[i+1]; ++k)
        z += a[k] * x[j[k]];
    y[i] += z;
}

inline void csr_spmv_inner_loop_regular_traffic(
    int i,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    double z = 0.0;
    for (int k = p[i]; k < p[i+1]; ++k)
        z += a[k];
    y[i] += z;
}

inline void csr_spmv_inner_loop_irregular_traffic(
    int i,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    double z = 0.0;
    for (int k = p[i]; k < p[i+1]; ++k)
        z += x[j[k]];
    y[i] += z;
}

inline void csr_spmv(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y,
    int chunk_size)
{
    #pragma omp for nowait schedule(static, chunk_size)
    for (int i = 0; i < m; ++i) {
        csr_spmv_inner_loop(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll2(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~1); i+=2) {
        csr_spmv_inner_loop(i, p, j, a, x, y);
        csr_spmv_inner_loop(i+1, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~1); i < m; ++i) {
        csr_spmv_inner_loop(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll4(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~3); i+=4) {
        csr_spmv_inner_loop(i, p, j, a, x, y);
        csr_spmv_inner_loop(i+1, p, j, a, x, y);
        csr_spmv_inner_loop(i+2, p, j, a, x, y);
        csr_spmv_inner_loop(i+3, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~3); i < m; ++i) {
        csr_spmv_inner_loop(i, p, j, a, x, y);
    }
}

inline void csr_spmv_regular_traffic(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < m; ++i) {
        csr_spmv_inner_loop_regular_traffic(i, p, j, a, x, y);
    }
}

inline void csr_spmv_irregular_traffic(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < m; ++i) {
        csr_spmv_inner_loop_irregular_traffic(i, p, j, a, x, y);
    }
}

void csr_matrix::spmv(
    csr_matrix::Matrix const & A,
    csr_matrix::Matrix::value_array_type const & x,
    csr_matrix::Matrix::value_array_type & y,
    int chunk_size)
{
    if (chunk_size <= 0) {
        int num_threads = omp_get_num_threads();
        chunk_size = (A.rows + num_threads - 1) / num_threads;
    }

    csr_spmv(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data(), chunk_size);
}

void csr_matrix::spmv_unroll2(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll2(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_unroll4(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll4(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_regular_traffic(
    csr_matrix::Matrix const & A,
    csr_matrix::Matrix::value_array_type const & x,
    csr_matrix::Matrix::value_array_type & y)
{
    csr_spmv_regular_traffic(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_irregular_traffic(
    csr_matrix::Matrix const & A,
    csr_matrix::Matrix::value_array_type const & x,
    csr_matrix::Matrix::value_array_type & y)
{
    csr_spmv_irregular_traffic(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

#ifdef __AVX__
inline void csr_spmv_inner_loop_avx128(
    int i,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    __m64 j_;
    __m128d z_ = _mm_setzero_pd();
    __m128d a_;
    __m128d x_;

    int k = p[i];
    if ((k & 1) && (k < p[i+1])) {
        a_ = _mm_load_sd(&a[k]);
        x_ = _mm_load_sd(&x[j[k]]);
        z_ = _mm_mul_pd(x_, a_);
        k++;
    }

    for (; k < (p[i+1] & ~1); k += 2) {
        a_ = _mm_load_pd(&a[k]);
        x_ = _mm_set_pd(x[j[k+1]], x[j[k]]);
        x_ = _mm_mul_pd(x_, a_);
        z_ = _mm_add_pd(z_, x_);
    }

    if ((p[i+1] & 1) && (k < p[i+1])) {
        a_ = _mm_load_sd(&a[k]);
        x_ = _mm_load_sd(&x[j[k]]);
        x_ = _mm_mul_pd(x_, a_);
        z_ = _mm_add_pd(z_, x_);
    }

    z_ = _mm_hadd_pd(z_, z_);
    y[i] += _mm_cvtsd_f64(z_);
}

inline void csr_spmv_avx128(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < m; ++i) {
        csr_spmv_inner_loop_avx128(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll2_avx128(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~1); i+=2) {
        csr_spmv_inner_loop_avx128(i, p, j, a, x, y);
        csr_spmv_inner_loop_avx128(i+1, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~1); i < m; ++i) {
        csr_spmv_inner_loop_avx128(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll4_avx128(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~3); i+=4) {
        csr_spmv_inner_loop_avx128(i, p, j, a, x, y);
        csr_spmv_inner_loop_avx128(i+1, p, j, a, x, y);
        csr_spmv_inner_loop_avx128(i+2, p, j, a, x, y);
        csr_spmv_inner_loop_avx128(i+3, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~3); i < m; ++i) {
        csr_spmv_inner_loop_avx128(i, p, j, a, x, y);
    }
}

void csr_matrix::spmv_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_avx128(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_unroll2_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll2_avx128(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_unroll4_avx128(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll4_avx128(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}
#endif

#ifdef __AVX2__
inline void csr_spmv_inner_loop_avx256(
    int i,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    __m128i j_;
    __m256d a_;
    __m256d x_;
    __m256d z_ = _mm256_setzero_pd();
    int k = p[i];

    if ((k & 1) && (k < p[i+1])) {
        assert(k < p[i+1]);
        a_ = _mm256_castpd128_pd256(_mm_load_sd(&a[k]));
    	x_ = _mm256_castpd128_pd256(_mm_load_sd(&x[j[k]]));
        z_ = _mm256_mul_pd(x_, a_);
        k++;
    }
    if ((k & 2) && (k+1 < p[i+1])) {
        assert(k+1 < p[i+1]);
        a_ = _mm256_castpd128_pd256(_mm_load_pd(&a[k]));
        x_ = _mm256_castpd128_pd256(_mm_set_pd(x[j[k+1]], x[j[k]]));
        x_ = _mm256_mul_pd(x_, a_);
        z_ = _mm256_add_pd(z_, x_);
        k += 2;
    }

    for (; k < (p[i+1] & ~3); k += 4) {
        a_ = _mm256_load_pd(&a[k]);
        j_ = _mm_load_si128((__m128i *)&j[k]);
        x_ = _mm256_set_pd(
            x[_mm_extract_epi32(j_,3)],
            x[_mm_extract_epi32(j_,2)],
            x[_mm_extract_epi32(j_,1)],
            x[_mm_extract_epi32(j_,0)]);
        x_ = _mm256_mul_pd(x_, a_);
        z_ = _mm256_add_pd(z_, x_);
    }

    if ((p[i+1] & 2) && (k+1 < p[i+1])) {
        assert(k+1 < p[i+1]);
        a_ = _mm256_castpd128_pd256(_mm_load_pd(&a[k]));
        x_ = _mm256_castpd128_pd256(_mm_set_pd(x[j[k+1]], x[j[k]]));
        x_ = _mm256_mul_pd(x_, a_);
        z_ = _mm256_add_pd(z_, x_);
        k += 2;
    }
    if ((p[i+1] & 1) && (k < p[i+1])) {
        assert(k < p[i+1]);
        a_ = _mm256_castpd128_pd256(_mm_load_sd(&a[k]));
    	x_ = _mm256_castpd128_pd256(_mm_load_sd(&x[j[k]]));
        x_ = _mm256_mul_pd(x_, a_);
        z_ = _mm256_add_pd(z_, x_);
    }

    z_ = _mm256_hadd_pd(z_, z_);
    y[i] += _mm_cvtsd_f64(
        _mm_add_sd(
            _mm256_extractf128_pd(z_, 1),
            _mm256_castpd256_pd128(z_)));
}

inline void csr_spmv_avx256(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < m; ++i) {
        csr_spmv_inner_loop_avx256(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll2_avx256(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~1); i+=2) {
        csr_spmv_inner_loop_avx256(i, p, j, a, x, y);
        csr_spmv_inner_loop_avx256(i+1, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~1); i < m; ++i) {
        csr_spmv_inner_loop_avx256(i, p, j, a, x, y);
    }
}

inline void csr_spmv_unroll4_avx256(
    int m,
    int const * p,
    int const * j,
    double const * a,
    double const * x,
    double * y)
{
    #pragma omp for nowait
    for (int i = 0; i < (m & ~3); i+=4) {
        csr_spmv_inner_loop_avx256(i, p, j, a, x, y);
        csr_spmv_inner_loop_avx256(i+1, p, j, a, x, y);
        csr_spmv_inner_loop_avx256(i+2, p, j, a, x, y);
        csr_spmv_inner_loop_avx256(i+3, p, j, a, x, y);
    }

    #pragma omp single nowait
    for (int i = (m & ~3); i < m; ++i) {
        csr_spmv_inner_loop_avx256(i, p, j, a, x, y);
    }
}

void csr_matrix::spmv_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_avx256(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_unroll2_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll2_avx256(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}

void csr_matrix::spmv_unroll4_avx256(
    Matrix const & A,
    Matrix::value_array_type const & x,
    Matrix::value_array_type & y)
{
    csr_spmv_unroll4_avx256(
        A.rows, A.row_ptr.data(),
        A.column_index.data(), A.value.data(),
        x.data(), y.data());
}
#endif
