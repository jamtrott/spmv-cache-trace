#ifndef MATRIX_MARKET_HPP
#define MATRIX_MARKET_HPP

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace matrix_market
{

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
    std::size_t const rows;
    std::size_t const columns;
    std::size_t const numEntries;
};

struct CoordinateEntry
{
    std::size_t i;
    std::size_t j;
    double a;

    CoordinateEntry()
        : i(0u), j(0u), a(0.0)
    {
    }

    CoordinateEntry(std::size_t i, std::size_t j, double a)
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
    std::size_t rows() const;
    std::size_t columns() const;
    std::size_t numEntries() const;
    std::size_t maxRowLength() const;

    std::vector<std::size_t> rowLengths() const;

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

class parse_error : public std::runtime_error
{
public:
    parse_error(std::string const & s) throw();
};

Matrix fromStream(std::istream & i);

std::ostream & toStream(
    std::ostream & o,
    matrix_market::Matrix const & m,
    bool write_entries);

matrix_market::Matrix load_matrix(
    std::string const & path,
    std::ostream & o,
    bool verbose = false);

}

#endif
