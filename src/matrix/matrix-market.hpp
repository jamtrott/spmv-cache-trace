#ifndef MATRIX_MARKET_HPP
#define MATRIX_MARKET_HPP

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace matrix_market
{

typedef int32_t size_type;
typedef int32_t index_type;
typedef double real_type;

enum class Object { matrix };
enum class Format { coordinate, array };
enum class Field { real, complex, integer, pattern };
enum class Symmetry { general, symmetric, skew_symmetric, hermitian };

class Header
{
public:
    std::string identifier;
    Object object;
    Format format;
    Field field;
    Symmetry symmetry;
};

using Comments = std::vector<std::string>;

class Size
{
public:
    index_type rows;
    index_type columns;
    size_type num_entries;
};

struct CoordinateEntryReal
{
    index_type i;
    index_type j;
    real_type a;
};

struct CoordinateEntryComplex
{
    index_type i;
    index_type j;
    real_type real, imag;
};

struct CoordinateEntryInteger
{
    index_type i;
    index_type j;
    int a;
};

struct CoordinateEntryPattern
{
    index_type i;
    index_type j;
};

bool operator==(CoordinateEntryReal const & a, CoordinateEntryReal const & b);
bool operator==(CoordinateEntryComplex const & a, CoordinateEntryComplex const & b);
bool operator==(CoordinateEntryInteger const & a, CoordinateEntryInteger const & b);
bool operator==(CoordinateEntryPattern const & a, CoordinateEntryPattern const & b);

class Matrix
{
public:
    Matrix(Header const & header,
           Comments const & comments,
           Size const & size,
           std::vector<CoordinateEntryReal> const & entries);
    Matrix(Header const & header,
           Comments const & comments,
           Size const & size,
           std::vector<CoordinateEntryComplex> const & entries);
    Matrix(Header const & header,
           Comments const & comments,
           Size const & size,
           std::vector<CoordinateEntryInteger> const & entries);
    Matrix(Header const & header,
           Comments const & comments,
           Size const & size,
           std::vector<CoordinateEntryPattern> const & entries);

    Matrix(Matrix && m) = default;
    Matrix & operator=(Matrix && m) = default;

public:
    Header const & header() const;
    Format format() const;
    Field field() const;
    Symmetry symmetry() const;

    Comments const & comments() const;

    Size const & size() const;
    index_type rows() const;
    index_type columns() const;
    size_type num_entries() const;

    std::vector<CoordinateEntryReal> const & coordinate_entries_real() const;
    std::vector<CoordinateEntryComplex> const & coordinate_entries_complex() const;
    std::vector<CoordinateEntryInteger> const & coordinate_entries_integer() const;
    std::vector<CoordinateEntryPattern> const & coordinate_entries_pattern() const;

    std::vector<index_type> row_indices() const;
    std::vector<index_type> column_indices() const;
    std::vector<real_type> values_real() const;

    index_type max_row_length() const;
    std::vector<index_type> row_lengths() const;

    void permute (std::vector<int> const & new_order);

private:
    Header header_;
    Comments comments_;
    Size size_;
    std::vector<CoordinateEntryReal> entries_real;
    std::vector<CoordinateEntryComplex> entries_complex;
    std::vector<CoordinateEntryInteger> entries_integer;
    std::vector<CoordinateEntryPattern> entries_pattern;
};

bool operator==(Matrix const & A, Matrix const & B);
std::ostream & operator<<(std::ostream & o, matrix_market::Matrix const & m);

Matrix fromStream(std::istream & i);

std::ostream & toStream(
    std::ostream & o,
    matrix_market::Matrix const & m,
    bool write_entries);

matrix_market::Matrix load_matrix(
    std::string const & path,
    std::ostream & o,
    bool verbose = false);

matrix_market::Matrix sort_matrix_column_major(
    matrix_market::Matrix const & m);

matrix_market::Matrix sort_matrix_row_major(
    matrix_market::Matrix const & m);

}

#endif
