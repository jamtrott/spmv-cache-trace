#include "poisson2D.hpp"

#include "matrix/csr-matrix.hpp"
#include "matrix/matrix-market.hpp"
#include "vector.hpp"

#include <gtest/gtest.h>

#include <omp.h>

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

namespace
{

csr_matrix::Matrix testMatrix()
{
    csr_matrix::index_array_type row_ptr{{0, 2, 3, 4, 7}};
    csr_matrix::index_array_type column_index{{0, 1, 1, 2, 0, 3, 4}};
    csr_matrix::value_array_type value{{1.0, 2.0, 1.0, 3.0, -1.0, 2.0, 1.0}};
    return csr_matrix::Matrix(
        4, 5, 7, 1, row_ptr, column_index, value);
}

csr_matrix::Matrix testMatrixRowAligned()
{
    csr_matrix::index_array_type row_ptr{{0, 2, 4, 6, 10}};
    int pad = 0;
    csr_matrix::index_array_type column_index{{0, 1, 1, pad, 2, pad, 0, 3, 4, pad}};
    double vpad = 0.0;
    csr_matrix::value_array_type value{{1.0, 2.0, 1.0, vpad, 3.0, vpad, -1.0, 2.0, 1.0, vpad}};
    return csr_matrix::Matrix(
        4, 5, 7, 1, row_ptr, column_index, value);
}

}

TEST(csr_matrix, create)
{
    auto m = testMatrix();
    ASSERT_EQ(m.rows, 4);
    ASSERT_EQ(m.columns, 5);
    ASSERT_EQ(m.num_entries, 7);
    csr_matrix::index_array_type row_ptr{{0, 2, 3, 4, 7}};
    ASSERT_EQ(m.row_ptr, row_ptr);
    csr_matrix::index_array_type column_index = {{0,1, 1, 2, 0, 3, 4}};
    ASSERT_EQ(m.column_index, column_index);
    csr_matrix::value_array_type value = {{1.0, 2.0, 1.0, 3.0, -1.0, 2.0, 1.0}};
    ASSERT_EQ(m.value, value);
}

TEST(csr_matrix, from_matrix_market)
{
    auto s = std::string{
        "%%MatrixMarket matrix coordinate real general\n"
        "% Test matrix\n"
        "4 5 7\n"
        "1 1 1.0\n"
        "1 2 2.0\n"
        "2 2 1.0\n"
        "3 3 3.0\n"
        "4 1 -1.0\n"
        "4 4 2.0\n"
        "4 5 1.0\n"};
    std::istringstream stream{s};
    auto mm = matrix_market::fromStream(stream);
    auto m = csr_matrix::from_matrix_market(mm);
    ASSERT_EQ(m, testMatrix());
}

TEST(csr_matrix, from_matrix_market_row_aligned)
{
    auto s = std::string{
        "%%MatrixMarket matrix coordinate real general\n"
        "% Test matrix\n"
        "4 5 7\n"
        "1 1 1.0\n"
        "1 2 2.0\n"
        "2 2 1.0\n"
        "3 3 3.0\n"
        "4 1 -1.0\n"
        "4 4 2.0\n"
        "4 5 1.0\n"};
    std::istringstream stream{s};
    auto mm = matrix_market::fromStream(stream);
    auto m = csr_matrix::from_matrix_market_row_aligned(mm, 2);
    ASSERT_EQ(m, testMatrixRowAligned());
}

TEST(csr_matrix, matrix_vector_multiplication)
{
    auto A = testMatrix();
    auto x = csr_matrix::value_array_type{{5.0, 2.0, 3.0, 3.0, 1.0}};
    auto y = A * x;
    auto z = csr_matrix::value_array_type{{9.0, 2.0, 9.0, 2.0}};
    ASSERT_DOUBLE_EQ(l2norm(y - z), 0.0);
}

TEST(csr_matrix, poisson2D)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = A * x;
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll2)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll2(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll4)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll4(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_row_aligned)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market_row_aligned(mm, 2u);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = A * x;
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_parallel)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);

    #pragma omp parallel
    {
        omp_set_num_threads(2);
        csr_matrix::spmv(A, x, y);
    }

    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

#ifdef __AVX__
TEST(csr_matrix, poisson2D_avx128)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_avx128(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll2_avx128)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll2_avx128(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll4_avx128)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll4_avx128(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}
#endif

#ifdef __AVX2__
TEST(csr_matrix, poisson2D_avx256)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_avx256(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll2_avx256)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll2_avx256(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(csr_matrix, poisson2D_unroll4_avx256)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_unroll4_avx256(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}
#endif

#ifdef USE_INTEL_MKL
TEST(csr_matrix, poisson2D_spmv_mkl)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = csr_matrix::from_matrix_market(mm);
    auto x = csr_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = csr_matrix::value_array_type(A.rows, 0.0);
    csr_matrix::spmv_mkl(A, x, y);
    auto z = csr_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}
#endif
