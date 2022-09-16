#include "poisson2D.hpp"

#include "matrix/ell-matrix.hpp"
#include "matrix/matrix-market.hpp"
#include "vector.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

ell_matrix::Matrix testMatrix()
{
    /*
     * [[1 2 0 3 0]
     *  [4 1 0 0 0]
     *  [0 0 3 0 0]
     *  [0 0 0 2 1]]
     */
    auto pad = 0.0;
    ell_matrix::index_array_type column_index{
        {0u, 1u, 3u, 0u, 1u, 1u, 2u, 2u, 2u, 3u, 4u, 4u}};
    ell_matrix::value_array_type value{
        {1.0, 2.0, 3.0, 4.0, 1.0, pad, 3.0, pad, pad, 2.0, 1.0, pad}};
    return ell_matrix::Matrix(
        4u, 5u, 8u, 3u, column_index, value);
}

}

TEST(ell_matrix, create)
{
    auto m = testMatrix();
    ASSERT_EQ(m.rows, 4u);
    ASSERT_EQ(m.columns, 5u);
    ASSERT_EQ(m.num_entries, 8u);
    ASSERT_EQ(m.row_length, 3u);
    ell_matrix::index_array_type column_index{
        {0u, 1u, 3u, 0u, 1u, 1u, 2u, 2u, 2u, 3u, 4u, 4u}};
    ASSERT_EQ(m.column_index, column_index);
    auto pad = 0.0;
    ell_matrix::value_array_type value{
        {1.0, 2.0, 3.0, 4.0, 1.0, pad, 3.0, pad, pad, 2.0, 1.0, pad}};
    ASSERT_EQ(m.value, value);
}

TEST(ell_matrix, from_matrix_market)
{
    auto s = std::string{
        "%%MatrixMarket matrix coordinate real general\n"
        "% Test matrix\n"
        "4 5 8\n"
        "1 1 1.0\n"
        "1 2 2.0\n"
        "1 4 3.0\n"
        "2 1 4.0\n"
        "2 2 1.0\n"
        "3 3 3.0\n"
        "4 4 2.0\n"
        "4 5 1.0\n"};
    std::istringstream stream{s};
    auto mm = matrix_market::fromStream(stream);
    auto m = ell_matrix::from_matrix_market(mm);
    ASSERT_EQ(m, testMatrix());
}

TEST(ell_matrix, matrix_vector_multiplication)
{
    auto A = testMatrix();
    auto x = ell_matrix::value_array_type{5.0, 2.0, 3.0, 3.0, 1.0};
    auto y = A * x;
    auto z = ell_matrix::value_array_type{18.0, 22.0, 9.0, 7.0};
    ASSERT_DOUBLE_EQ(l2norm(y - z), 0.0)
        << "A = " << A << ",\n" << "x = " << x << ",\n"
        << "y = " << y << ",\n" << "z = " << z;
}

TEST(ell_matrix, poisson2D)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = ell_matrix::from_matrix_market(mm);
    auto x = ell_matrix::value_array_type{
        std::begin(poisson2D_b), std::end(poisson2D_b)};
    auto y = A * x;
    auto z = ell_matrix::value_array_type{
        std::begin(poisson2D_result), std::end(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(ell_matrix, aligned_arrays)
{
    auto A = testMatrix();
    ASSERT_EQ(0u, intptr_t(A.column_index.data()) % 64);
    ASSERT_EQ(0u, intptr_t(A.value.data()) % 64);
}
