#include "poisson2D.hpp"

#include "matrix/coo-matrix.hpp"
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

coo_matrix::Matrix testMatrix()
{
    coo_matrix::index_array_type row_index{{0, 0, 1, 2, 3, 3}};
    coo_matrix::index_array_type column_index{{0, 1, 1, 2, 3, 4}};
    coo_matrix::value_array_type value{{1.0, 2.0, 1.0, 3.0, 2.0, 1.0}};
    return coo_matrix::Matrix(
        4, 5, 6, row_index, column_index, value);
}

}

TEST(coo_matrix, create)
{
    auto m = testMatrix();
    ASSERT_EQ(m.rows, 4);
    ASSERT_EQ(m.columns, 5);
    ASSERT_EQ(m.num_entries, 6);
    coo_matrix::index_array_type row_index{{0, 0, 1, 2, 3, 3}};
    ASSERT_EQ(m.row_index, row_index);
    coo_matrix::index_array_type column_index = {{0, 1, 1, 2, 3, 4}};
    ASSERT_EQ(m.column_index, column_index);
    coo_matrix::value_array_type value = {{1.0, 2.0, 1.0, 3.0, 2.0, 1.0}};
    ASSERT_EQ(m.value, value);
}

TEST(coo_matrix, from_matrix_market)
{
    auto s = std::string{
        "%%MatrixMarket matrix coordinate real general\n"
        "% Test matrix\n"
        "4 5 6\n"
        "1 1 1.0\n"
        "1 2 2.0\n"
        "2 2 1.0\n"
        "3 3 3.0\n"
        "4 4 2.0\n"
        "4 5 1.0\n"};
    std::istringstream stream{s};
    auto mm = matrix_market::fromStream(stream);
    auto m = coo_matrix::from_matrix_market(mm);
    ASSERT_EQ(m, testMatrix());
}

TEST(coo_matrix, matrix_vector_multiplication)
{
    auto A = testMatrix();
    auto x = coo_matrix::value_array_type{{5.0, 2.0, 3.0, 3.0, 1.0}};
    auto y = A * x;
    auto z = coo_matrix::value_array_type{{9.0, 2.0, 9.0, 7.0}};
    ASSERT_DOUBLE_EQ(l2norm(y - z), 0.0);
}

namespace
{

coo_matrix::Matrix testMatrixColumnMajor()
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
    return coo_matrix::from_matrix_market(mm);
}

}

TEST(coo_matrix, matrix_vector_multiplication_column_major)
{
    auto A = testMatrixColumnMajor();
    auto x = coo_matrix::value_array_type{{5.0, 2.0, 3.0, 3.0, 1.0}};
    auto y = A * x;
    auto z = coo_matrix::value_array_type{{18.0, 22.0, 9.0, 7.0}};
    ASSERT_DOUBLE_EQ(l2norm(y - z), 0.0)
        << "A = " << A << ",\n" << "x = " << x << ",\n"
        << "y = " << y << ",\n" << "z = " << z;
}

TEST(coo_matrix, poisson2D)
{
    std::istringstream stream{poisson2D};
    auto mm_unsorted = matrix_market::fromStream(stream);
    auto mm = matrix_market::sort_matrix_row_major(mm_unsorted);
    auto A = coo_matrix::from_matrix_market(mm);
    auto x = coo_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = A * x;
    auto z = coo_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}

TEST(coo_matrix, poisson2D_parallel)
{
    std::istringstream stream{poisson2D};
    auto mm_unsorted = matrix_market::fromStream(stream);
    auto mm = matrix_market::sort_matrix_row_major(mm_unsorted);
    auto A = coo_matrix::from_matrix_market(mm);
    auto x = coo_matrix::value_array_type{
        std::cbegin(poisson2D_b), std::cend(poisson2D_b)};
    auto y = coo_matrix::value_array_type(A.rows, 0.0);

    int num_threads = 2;
    omp_set_num_threads(num_threads);
    auto workspace = coo_matrix::value_array_type(num_threads*A.rows, 0.0);
    #pragma omp parallel
    {
        coo_matrix::spmv(num_threads, A, x, y, workspace);
    }

    auto z = coo_matrix::value_array_type{
        std::cbegin(poisson2D_result), std::cend(poisson2D_result)};
    ASSERT_NEAR(l2norm(y - z), 0.0, std::numeric_limits<double>::epsilon());
}
