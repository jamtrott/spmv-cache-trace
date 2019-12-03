#include "poisson2D.hpp"

#include "matrix/hybrid-matrix.hpp"
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

hybrid_matrix::Matrix testMatrix()
{
    /*
     * [[1 2 0 3 5]
     *  [4 1 0 0 0]
     *  [0 0 3 0 0]
     *  [0 0 0 2 1]]
     */
    auto pad = 0.0;
    hybrid_matrix::index_array_type ellpack_column_index{
        {0u, 1u, 3u, 0u, 1u, 1u, 2u, 2u, 2u, 3u, 4u, 4u}};
    hybrid_matrix::value_array_type ellpack_value{
        {1.0, 2.0, 3.0, 4.0, 1.0, pad, 3.0, pad, pad, 2.0, 1.0, pad}};

    hybrid_matrix::size_type num_coo_entries = 1;
    hybrid_matrix::index_array_type coo_row_index{0};
    hybrid_matrix::index_array_type coo_column_index{4};
    hybrid_matrix::value_array_type coo_value{5};

    return hybrid_matrix::Matrix(
        4u, 5u, 8u, 3u, ellpack_column_index, ellpack_value, false,
        num_coo_entries, coo_row_index, coo_column_index, coo_value);
}

}

TEST(hybrid_matrix, create)
{
    auto m = testMatrix();
    ASSERT_EQ(m.rows, 4u);
    ASSERT_EQ(m.columns, 5u);
    ASSERT_EQ(m.num_entries, 8u);
    ASSERT_EQ(m.ellpack_row_length, 3u);
    hybrid_matrix::index_array_type ellpack_column_index{
        {0u, 1u, 3u, 0u, 1u, 1u, 2u, 2u, 2u, 3u, 4u, 4u}};
    ASSERT_EQ(m.ellpack_column_index, ellpack_column_index);
    auto pad = 0.0;
    hybrid_matrix::value_array_type ellpack_value{
        {1.0, 2.0, 3.0, 4.0, 1.0, pad, 3.0, pad, pad, 2.0, 1.0, pad}};
    ASSERT_EQ(m.ellpack_value, ellpack_value);
}

TEST(hybrid_matrix, from_matrix_market)
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
    auto m = hybrid_matrix::from_matrix_market(mm);
    ASSERT_EQ(m, testMatrix());
}

TEST(hybrid_matrix, matrix_vector_multiplication)
{
    auto A = testMatrix();
    auto x = hybrid_matrix::value_array_type{5.0, 2.0, 3.0, 3.0, 1.0};
    auto y = A * x;
    auto z = hybrid_matrix::value_array_type{23.0, 22.0, 9.0, 7.0};
    ASSERT_DOUBLE_EQ(l2norm(y - z), 0.0)
        << "A = " << A << ",\n" << "x = " << x << ",\n"
        << "y = " << y << ",\n" << "z = " << z;
}

TEST(hybrid_matrix, poisson2D)
{
    std::istringstream stream{poisson2D};
    auto mm = matrix_market::fromStream(stream);
    auto A = hybrid_matrix::from_matrix_market(mm);
    auto x = hybrid_matrix::value_array_type{
        std::begin(poisson2D_b), std::end(poisson2D_b)};
    auto y = A * x;
    auto z = hybrid_matrix::value_array_type{
        std::begin(poisson2D_result), std::end(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(hybrid_matrix, aligned_arrays)
{
    auto A = testMatrix();
    ASSERT_EQ(0u, intptr_t(A.ellpack_column_index.data()) % 64);
    ASSERT_EQ(0u, intptr_t(A.ellpack_value.data()) % 64);
}
