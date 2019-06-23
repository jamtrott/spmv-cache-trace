#include "matrix-market.hpp"
#include "util/zlibstream.hpp"
#include "util/tarstream.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <errno.h>
#include <string.h>

using namespace matrix_market;
using namespace std::literals::string_literals;

bool matrix_market::operator==(
    CoordinateEntry const & a,
    CoordinateEntry const & b)
{
    return a.i == b.i && a.j == b.j && a.a == b.a;
}

index_type Matrix::rows() const
{
    return size.rows;
}

index_type Matrix::columns() const
{
    return size.columns;
}

size_type Matrix::numEntries() const
{
    return size.numEntries;
}

/*
 * Compute the length of the longest row
 */
index_type Matrix::maxRowLength() const
{
    auto row_lengths = rowLengths();
    return *std::max_element(
        std::cbegin(row_lengths), std::cend(row_lengths));
}

/*
 * Compute the length of each row
 */
std::vector<index_type> Matrix::rowLengths() const
{
    std::vector<index_type> row_lengths(size.rows, 0u);
    std::for_each(
        std::cbegin(entries), std::cend(entries),
        [&row_lengths] (auto const & a) { ++row_lengths[a.i-1u]; });
    return row_lengths;
}

/*
 * Obtain a copy of the matrix entries sorted in row- or column-major order
 */
std::vector<CoordinateEntry> Matrix::sortedCoordinateEntries(
    Order o) const
{
    auto sorted_entries = entries;
    if (o == Order::column_major) {
        std::sort(
            std::begin(sorted_entries), std::end(sorted_entries),
            [] (auto const & a, auto const & b)
            { return std::tie(a.j, a.i) < std::tie(b.j, b.i); });
    } else if (o == Order::row_major) {
        std::sort(
            std::begin(sorted_entries), std::end(sorted_entries),
            [] (auto const & a, auto const & b)
            { return std::tie(a.i, a.j) < std::tie(b.i, b.j); });
    }
    return sorted_entries;
}

matrix_market_error::matrix_market_error(std::string const & s) throw()
    : std::runtime_error(s)
{}

namespace
{

Object readObject(std::istream & i)
{
    std::string object;
    i >> object;

    if (object != std::string{"matrix"}) {
        auto s = std::stringstream{};
        s << "Failed to parse header: "
          << "Expected \"matrix\", got \"" << object << "\"";
        throw matrix_market_error{s.str()};
    }

    return Object::matrix;
}

Format readFormat(std::istream & i) {
    std::string format;
    i >> format;

    if (format == std::string{"coordinate"}) {
        return Format::coordinate;
    } else if (format == std::string{"array"}) {
        return Format::array;
    }

    auto s = std::stringstream{};
    s << "Expected \"coordinate\" or \"array\", got \"" << format << "\"";
    throw matrix_market_error{s.str()};
}

Header readHeader(std::istream & i)
{
    std::string header;
    std::getline(i, header);

    auto s = std::istringstream{header};
    std::string identifier, field, symmetry;
    s >> identifier;

    if (identifier != std::string{"%%MatrixMarket"}) {
        auto s = std::stringstream{};
        s << "Failed to parse header: "
          << "Expected \"%%MatrixMarket\", got \"" << identifier << "\"";
        throw matrix_market_error{s.str()};
    }

    auto object = readObject(s);
    auto format = readFormat(s);
    s >> field >> symmetry;

    return Header{identifier, object, format, field, symmetry};
}

std::vector<std::string> readComments(std::istream & i)
{
    std::vector<std::string> comments;
    while (i.peek() == '%') {
        std::string comment;
        std::getline(i, comment);
        comments.push_back(comment);
    }
    return comments;
}

Size readSize(std::istream & i, Format format)
{
    std::string line;
    if (!std::getline(i, line))
        throw matrix_market_error{"Failed to parse size"};

    std::stringstream s{line};
    if (format == Format::coordinate) {
        index_type rows, columns;
        size_type numEntries;
        s >> rows >> columns >> numEntries;
        return Size{rows, columns, numEntries};
    } else {
        index_type rows, columns;
        s >> rows >> columns;
        return Size{rows, columns, 0u};
    }
}

std::vector<CoordinateEntry> readEntries(
    std::istream & i,
    Size const & size)
{
    std::vector<CoordinateEntry> entries;
    entries.reserve(size.numEntries);
    std::copy_n(
        std::istream_iterator<CoordinateEntry>(i),
        size.numEntries,
        std::back_inserter(entries));

    if (size.numEntries != (size_type) entries.size()) {
        std::stringstream s;
        s << "Failed to parse entries: "
          << "Expected " << size.numEntries << " entries, "
          << "got " << entries.size() << " entries.";
        throw matrix_market_error{s.str()};
    }
    return entries;
}

}

namespace matrix_market {

std::istream & operator>>(std::istream & i, CoordinateEntry & e)
{
    return i >> e.i >> e.j >> e.a;
}

Matrix fromStream(std::istream & i)
{
    auto header = readHeader(i);
    auto comments = readComments(i);
    auto size = readSize(i, header.format);
    auto entries = readEntries(i, size);
    return Matrix{header, comments, size, entries};
}

}

std::string toString(Object const &)
{
    return std::string{"matrix"};
}

std::string toString(Format const & f)
{
    return std::string{f == Format::coordinate ? "coordinate" : "array"};
}

namespace matrix_market
{

bool operator==(Header const & A, Header const & B)
{
    return A.identifier == B.identifier &&
        A.object == B.object &&
        A.format == B.format &&
        A.field == B.field &&
        A.symmetry == B.symmetry;
}

bool operator==(
    Size const & A,
    Size const & B)
{
    return A.rows == B.rows &&
        A.columns == B.columns &&
        A.numEntries == B.numEntries;
}

bool operator==(Matrix const & A, Matrix const & B)
{
    return A.header == B.header &&
        A.comments == B.comments &&
        A.size == B.size &&
        A.entries == B.entries;
}

std::ostream & operator<<(std::ostream & o, Header const & h)
{
    return o << h.identifier << ' '
             << toString(h.object) << ' '
             << toString(h.format) << ' '
             << h.field << ' '
             << h.symmetry;
}

std::ostream & operator<<(
    std::ostream & o,
    std::vector<std::string> const & comments)
{
    std::copy(
        std::cbegin(comments),
        std::cend(comments),
        std::ostream_iterator<std::string>(o, "\n"));
    return o;
}

std::ostream & operator<<(
    std::ostream & o,
    Size const & s)
{
    return o << s.rows << ' ' << s.columns << ' ' << s.numEntries;
}

std::ostream & operator<<(
    std::ostream & o,
    CoordinateEntry const & e)
{
    return o << e.i << ' ' << e.j << ' ' << e.a;
}

std::ostream & toStream(
    std::ostream & o,
    Matrix const & m,
    bool write_entries)
{
    o << m.header << '\n'
      << m.comments
      << m.size << '\n';
    if (write_entries) {
        std::copy(
            std::cbegin(m.entries),
            std::cend(m.entries),
            std::ostream_iterator<CoordinateEntry>(o, "\n"));
    }
    return o;
}

template <typename T>
std::ostream & operator<<(
    std::ostream & o,
    std::vector<T> const & v)
{
    if (v.size() == 0u)
        return o << "()";

    o << '(';
    std::copy(std::begin(v), std::end(v) - 1u,
              std::ostream_iterator<T>(o, ", "));
    return o << v[v.size()-1u] << ')';
}

std::ostream & operator<<(std::ostream & o, Matrix const & m)
{
    return o
        << m.header << ' '
        << m.comments << ' '
        << m.size << ' '
        << m.entries;
}

bool ends_with(std::string const & s, std::string const & t)
{
    return (s.size() > t.size()) &&
        std::equal(t.rbegin(), t.rend(), s.rbegin());
}

matrix_market::Matrix load_compressed_matrix(
    std::ifstream & f,
    std::string const & path,
    std::string const & extension,
    std::ostream & o,
    bool verbose)
{
    auto start = path.find_last_of('/');
    start = (start == std::string::npos) ? 0ul : start + 1ul;
    auto end = path.rfind(extension);
    auto matrix_name = path.substr(start, end - start);
    auto filename = matrix_name + "/"s + matrix_name + ".mtx"s;
    if (verbose)
        o << "Loading compressed matrix from " << path << ':' << filename << '\n';

    try {
        zlib::izlibstream gzstream(f.rdbuf());
        tar::itarstream stream(gzstream.rdbuf(), filename);
        return matrix_market::fromStream(stream);
    } catch (zlib::zlibstream_error const & e) {
        throw matrix_market::matrix_market_error(e.what());
    }
}

matrix_market::Matrix load_matrix(
    std::string const & path,
    std::ostream & o,
    bool verbose)
{
    if (verbose)
        o << "Loading matrix from " << path << '\n';
    auto f = std::ifstream{path};
    if (!f)
        throw matrix_market::matrix_market_error(strerror(errno));

    if (ends_with(path, ".tar.gz"s)) {
        return load_compressed_matrix(f, path, ".tar.gz"s, o, verbose);
    } else if (ends_with(path, ".tgz"s)) {
        return load_compressed_matrix(f, path, ".tgz"s, o, verbose);
    } else {
        return matrix_market::fromStream(f);
    }
}

}
