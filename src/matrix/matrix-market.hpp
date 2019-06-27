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
typedef double value_type;

enum class Object { matrix };
enum class Format { coordinate, array };

class Header
{
public:
    std::string const identifier;
    Object const object;
    Format const format;
    std::string const field;
    std::string const symmetry;
};

using Comments = std::vector<std::string>;

class Size
{
public:
    index_type const rows;
    index_type const columns;
    size_type const num_entries;
};

struct CoordinateEntry
{
    index_type i;
    index_type j;
    value_type a;

    CoordinateEntry()
        : i(0u), j(0u), a(0.0)
    {
    }

    CoordinateEntry(index_type i, index_type j, value_type a)
        : i(i), j(j), a(a)
    {
    }
};

bool operator==(
    CoordinateEntry const & a,
    CoordinateEntry const & b);

using Entries = std::vector<CoordinateEntry>;

class Matrix
{
public:
    index_type rows() const;
    index_type columns() const;
    size_type num_entries() const;
    index_type maxRowLength() const;

    std::vector<index_type> row_lengths() const;

    enum class Order { column_major, row_major };
    std::vector<CoordinateEntry> sortedCoordinateEntries(Order o) const;

public:
    Header const header;
    std::vector<std::string> const comments;
    Size const size;
    Entries entries;
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

class matrix_market_error
    : public std::runtime_error
{
public:
    matrix_market_error(std::string const & s) throw();
};

}

#endif
