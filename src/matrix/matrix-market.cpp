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

#include <cfloat>
#include <cmath>
#include <cstring>
#include <errno.h>

using namespace matrix_market;
using namespace std::literals::string_literals;

namespace matrix_market {

bool operator==(CoordinateEntryReal const & a, CoordinateEntryReal const & b)
{
    return a.i == b.i && a.j == b.j && (fabs(a.a - b.a) < DBL_EPSILON);
}

bool operator==(CoordinateEntryComplex const & a, CoordinateEntryComplex const & b)
{
    return a.i == b.i && a.j == b.j &&
        (fabs(a.real - b.real) < DBL_EPSILON) &&
        (fabs(a.imag - b.imag) < DBL_EPSILON);
}

bool operator==(CoordinateEntryInteger const & a, CoordinateEntryInteger const & b)
{
    return a.i == b.i && a.j == b.j && a.a == b.a;
}

bool operator==(CoordinateEntryPattern const & a, CoordinateEntryPattern const & b)
{
    return a.i == b.i && a.j == b.j;
}

Matrix::Matrix(Header const & header,
               Comments const & comments,
               Size const & size,
               std::vector<CoordinateEntryReal> const & entries)
    : header_(header)
    , comments_(comments)
    , size_(size)
    , entries_real(entries)
    , entries_complex()
    , entries_integer()
    , entries_pattern()
{
}

Matrix::Matrix(Header const & header,
               Comments const & comments,
               Size const & size,
               std::vector<CoordinateEntryComplex> const & entries)
    : header_(header)
    , comments_(comments)
    , size_(size)
    , entries_real()
    , entries_complex(entries)
    , entries_integer()
    , entries_pattern()
{
}

Matrix::Matrix(Header const & header,
               Comments const & comments,
               Size const & size,
               std::vector<CoordinateEntryInteger> const & entries)
    : header_(header)
    , comments_(comments)
    , size_(size)
    , entries_real()
    , entries_complex()
    , entries_integer(entries)
    , entries_pattern()
{
}

Matrix::Matrix(Header const & header,
               Comments const & comments,
               Size const & size,
               std::vector<CoordinateEntryPattern> const & entries)
    : header_(header)
    , comments_(comments)
    , size_(size)
    , entries_real()
    , entries_complex()
    , entries_integer()
    , entries_pattern(entries)
{
}

Header const & Matrix::header() const
{
    return header_;
}

Comments const & Matrix::comments() const
{
    return comments_;
}

Size const & Matrix::size() const
{
    return size_;
}

Format Matrix::format() const
{
    return header_.format;
}

Field Matrix::field() const
{
    return header_.field;
}

Symmetry Matrix::symmetry() const
{
    return header_.symmetry;
}

index_type Matrix::rows() const
{
    return size_.rows;
}

index_type Matrix::columns() const
{
    return size_.columns;
}

size_type Matrix::num_entries() const
{
    return size_.num_entries;
}

std::vector<CoordinateEntryReal> const & Matrix::coordinate_entries_real() const
{
    return entries_real;
}

std::vector<CoordinateEntryComplex> const & Matrix::coordinate_entries_complex() const
{
    return entries_complex;
}

std::vector<CoordinateEntryInteger> const & Matrix::coordinate_entries_integer() const
{
    return entries_integer;
}

std::vector<CoordinateEntryPattern> const & Matrix::coordinate_entries_pattern() const
{
    return entries_pattern;
}

std::vector<index_type> Matrix::row_indices() const
{
    std::vector<index_type> row_indices_(size_.num_entries);
    switch (header_.field) {
    case Field::real:
        {
            auto entries = coordinate_entries_real();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                row_indices_[k] = entries[k].i;
            break;
        }
    case Field::complex:
        {
            auto entries = coordinate_entries_complex();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                row_indices_[k] = entries[k].i;
            break;
        }
    case Field::integer:
        {
            auto entries = coordinate_entries_integer();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                row_indices_[k] = entries[k].i;
            break;
        }
    case Field::pattern:
        {
            auto entries = coordinate_entries_pattern();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                row_indices_[k] = entries[k].i;
            break;
        }
    }
    return row_indices_;
}

std::vector<index_type> Matrix::column_indices() const
{
    std::vector<index_type> column_indices_(size_.num_entries);
    switch (header_.field) {
    case Field::real:
        {
            auto entries = coordinate_entries_real();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                column_indices_[k] = entries[k].j;
            break;
        }
    case Field::complex:
        {
            auto entries = coordinate_entries_complex();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                column_indices_[k] = entries[k].j;
            break;
        }
    case Field::integer:
        {
            auto entries = coordinate_entries_integer();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                column_indices_[k] = entries[k].j;
            break;
        }
    case Field::pattern:
        {
            auto entries = coordinate_entries_pattern();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                column_indices_[k] = entries[k].j;
            break;
        }
    }
    return column_indices_;
}

std::vector<real_type> Matrix::values_real() const
{
    std::vector<real_type> values_(size_.num_entries);
    switch (header_.field) {
    case Field::real:
        {
            auto entries = coordinate_entries_real();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                values_[k] = entries[k].a;
            break;
        }
    case Field::complex:
        {
            auto entries = coordinate_entries_complex();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                values_[k] = entries[k].real;
            break;
        }
    case Field::integer:
        {
            auto entries = coordinate_entries_integer();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                values_[k] = entries[k].a;
            break;
        }
    case Field::pattern:
        {
            auto entries = coordinate_entries_pattern();
            for (size_type k = 0; k < (size_type) entries.size(); k++)
                values_[k] = 1.0;
            break;
        }
    }
    return values_;
}

/*
 * Compute the length of the longest row
 */
index_type Matrix::max_row_length() const
{
    auto const & v = row_lengths();
    return *std::max_element(std::cbegin(v), std::cend(v));
}

/*
 * Compute the length of each row
 */
std::vector<index_type> Matrix::row_lengths() const
{
    std::vector<index_type> row_lengths_(size_.rows, 0u);
    std::for_each(
        std::cbegin(entries_real), std::cend(entries_real),
        [&row_lengths_] (auto const & a) { ++row_lengths_[a.i-1u]; });
    std::for_each(
        std::cbegin(entries_complex), std::cend(entries_complex),
        [&row_lengths_] (auto const & a) { ++row_lengths_[a.i-1u]; });
    std::for_each(
        std::cbegin(entries_integer), std::cend(entries_integer),
        [&row_lengths_] (auto const & a) { ++row_lengths_[a.i-1u]; });
    std::for_each(
        std::cbegin(entries_pattern), std::cend(entries_pattern),
        [&row_lengths_] (auto const & a) { ++row_lengths_[a.i-1u]; });
    return row_lengths_;
}

matrix_market_error::matrix_market_error(std::string const & s) throw()
    : std::runtime_error(s)
{}

}

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

Format readFormat(std::istream & i)
{
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

Field readField(std::istream & i)
{
    std::string field;
    i >> field;
    if (field == std::string("real")) {
        return Field::real;
    } else if (field == std::string("complex")) {
        return Field::complex;
    } else if (field == std::string("integer")) {
        return Field::integer;
    } else if (field == std::string("pattern")) {
        return Field::pattern;
    }
    auto s = std::stringstream{};
    s << "Expected \"real\", \"complex\", \"integer\", or \"pattern\", "
      << "got \"" << field << "\"";
    throw matrix_market_error{s.str()};
}

Symmetry readSymmetry(std::istream & i)
{
    std::string symmetry;
    i >> symmetry;
    if (symmetry == std::string{"general"}) {
        return Symmetry::general;
    } else if (symmetry == std::string{"symmetric"}) {
        return Symmetry::symmetric;
    } else if (symmetry == std::string{"skew-symmetric"}) {
        return Symmetry::skew_symmetric;
    } else if (symmetry == std::string{"hermitian"}) {
        return Symmetry::hermitian;
    }
    auto s = std::stringstream{};
    s << "Expected \"general\", \"symmetric\", \"skew-symmetric\", or \"hermitian\", "
      << "got \"" << symmetry << "\"";
    throw matrix_market_error{s.str()};
}

Header readHeader(std::istream & i)
{
    std::string header;
    std::getline(i, header);

    auto s = std::istringstream{header};
    std::string identifier;
    s >> identifier;
    if (identifier != std::string{"%%MatrixMarket"}) {
        auto s = std::stringstream{};
        s << "Failed to parse header: "
          << "Expected \"%%MatrixMarket\", got \"" << identifier << "\"";
        throw matrix_market_error{s.str()};
    }

    auto object = readObject(s);
    auto format = readFormat(s);
    auto field = readField(s);
    auto symmetry = readSymmetry(s);
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
        size_type num_entries;
        s >> rows >> columns >> num_entries;
        return Size{rows, columns, num_entries};
    } else {
        index_type rows, columns;
        s >> rows >> columns;
        return Size{rows, columns, 0u};
    }
}

}

namespace matrix_market {

std::istream & operator>>(std::istream & i, CoordinateEntryReal & e)
{
    return i >> e.i >> e.j >> e.a;
}

std::istream & operator>>(std::istream & i, CoordinateEntryComplex & e)
{
    return i >> e.i >> e.j >> e.real >> e.imag;
}

std::istream & operator>>(std::istream & i, CoordinateEntryInteger & e)
{
    return i >> e.i >> e.j >> e.a;
}

std::istream & operator>>(std::istream & i, CoordinateEntryPattern & e)
{
    return i >> e.i >> e.j;
}

template <typename T>
std::vector<T> readEntries(
    std::istream & i,
    size_type num_entries)
{
    std::vector<T> entries;
    entries.reserve(num_entries);
    std::copy_n(
        std::istream_iterator<T>(i),
        num_entries,
        std::back_inserter(entries));

    if (num_entries != (size_type) entries.size()) {
        std::stringstream s;
        s << "Failed to parse entries: "
          << "Expected " << num_entries << " entries, "
          << "got " << entries.size() << " entries.";
        throw matrix_market_error{s.str()};
    }
    return entries;
}

Matrix fromStream(std::istream & i)
{
    auto header = readHeader(i);
    auto comments = readComments(i);
    auto size = readSize(i, header.format);

    std::vector<CoordinateEntryReal> entries_real;
    std::vector<CoordinateEntryComplex> entries_complex;
    std::vector<CoordinateEntryInteger> entries_integer;
    std::vector<CoordinateEntryPattern> entries_pattern;
    switch (header.field) {
    case Field::real:
        entries_real = readEntries<CoordinateEntryReal>(i, size.num_entries);
        return Matrix(header, comments, size, entries_real);
    case Field::complex:
        entries_complex = readEntries<CoordinateEntryComplex>(i, size.num_entries);
        return Matrix(header, comments, size, entries_complex);
    case Field::integer:
        entries_integer = readEntries<CoordinateEntryInteger>(i, size.num_entries);
        return Matrix(header, comments, size, entries_integer);
    case Field::pattern:
        entries_pattern = readEntries<CoordinateEntryPattern>(i, size.num_entries);
        return Matrix(header, comments, size, entries_pattern);
    }
    throw matrix_market_error{"Unknown field"};
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

std::string toString(Field const & f)
{
    switch (f) {
    case Field::real: return std::string("real");
    case Field::complex: return std::string("complex");
    case Field::integer: return std::string("integer");
    case Field::pattern: return std::string("pattern");
    }
    throw matrix_market_error{"Unknown field"};
}

std::string toString(Symmetry const & s)
{
    switch (s) {
    case Symmetry::general: return std::string("generall");
    case Symmetry::symmetric: return std::string("symmetric");
    case Symmetry::skew_symmetric: return std::string("skew-symmetric");
    case Symmetry::hermitian: return std::string("hermitian");
    }
    throw matrix_market_error{"Unknown symmetry"};
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
        A.num_entries == B.num_entries;
}

bool operator==(Matrix const & A, Matrix const & B)
{
    return A.header() == B.header() &&
        A.comments() == B.comments() &&
        A.size() == B.size() &&
        A.coordinate_entries_real() == B.coordinate_entries_real() &&
        A.coordinate_entries_complex() == B.coordinate_entries_complex() &&
        A.coordinate_entries_integer() == B.coordinate_entries_integer() &&
        A.coordinate_entries_pattern() == B.coordinate_entries_pattern();
}

std::ostream & operator<<(std::ostream & o, Header const & h)
{
    return o << h.identifier << ' '
             << toString(h.object) << ' '
             << toString(h.format) << ' '
             << toString(h.field) << ' '
             << toString(h.symmetry);
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
    return o << s.rows << ' ' << s.columns << ' ' << s.num_entries;
}

std::ostream & operator<<(
    std::ostream & o,
    CoordinateEntryReal const & e)
{
    return o << e.i << ' ' << e.j << ' ' << e.a;
}

std::ostream & operator<<(
    std::ostream & o,
    CoordinateEntryComplex const & e)
{
    return o << e.i << ' ' << e.j << ' ' << e.real << ' ' << e.imag;
}

std::ostream & operator<<(
    std::ostream & o,
    CoordinateEntryInteger const & e)
{
    return o << e.i << ' ' << e.j << ' ' << e.a;
}

std::ostream & operator<<(
    std::ostream & o,
    CoordinateEntryPattern const & e)
{
    return o << e.i << ' ' << e.j;
}

std::ostream & toStream(
    std::ostream & o,
    Matrix const & m,
    bool write_entries)
{
    o << m.header() << '\n'
      << m.comments()
      << m.size() << '\n';
    if (write_entries) {
        switch (m.field()) {
        case Field::real:
            std::copy(std::cbegin(m.coordinate_entries_real()),
                      std::cend(m.coordinate_entries_real()),
                      std::ostream_iterator<CoordinateEntryReal>(o, "\n"));
            break;
        case Field::complex:
            std::copy(std::cbegin(m.coordinate_entries_complex()),
                      std::cend(m.coordinate_entries_complex()),
                      std::ostream_iterator<CoordinateEntryComplex>(o, "\n"));
            break;
        case Field::integer:
            std::copy(std::cbegin(m.coordinate_entries_integer()),
                      std::cend(m.coordinate_entries_integer()),
                      std::ostream_iterator<CoordinateEntryInteger>(o, "\n"));
            break;
        case Field::pattern:
            std::copy(std::cbegin(m.coordinate_entries_pattern()),
                      std::cend(m.coordinate_entries_pattern()),
                      std::ostream_iterator<CoordinateEntryPattern>(o, "\n"));
            break;
        }
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
    return toStream(o, m, true);
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

matrix_market::Matrix sort_matrix_column_major(
    matrix_market::Matrix const & m)
{
    auto compare_entries = [] (auto const & a, auto const & b)
        { return std::tie(a.j, a.i) < std::tie(b.j, b.i); };
    switch (m.field()) {
    case Field::real:
        {
            std::vector<CoordinateEntryReal> entries = m.coordinate_entries_real();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::complex:
        {
            std::vector<CoordinateEntryComplex> entries = m.coordinate_entries_complex();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::integer:
        {
            std::vector<CoordinateEntryInteger> entries = m.coordinate_entries_integer();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::pattern:
        {
            std::vector<CoordinateEntryPattern> entries = m.coordinate_entries_pattern();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    }
    throw matrix_market_error{"Unknown field"};
}

matrix_market::Matrix sort_matrix_row_major(
    matrix_market::Matrix const & m)
{
    auto compare_entries = [] (auto const & a, auto const & b)
        { return std::tie(a.i, a.j) < std::tie(b.i, b.j); };
    switch (m.field()) {
    case Field::real:
        {
            std::vector<CoordinateEntryReal> entries = m.coordinate_entries_real();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::complex:
        {
            std::vector<CoordinateEntryComplex> entries = m.coordinate_entries_complex();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::integer:
        {
            std::vector<CoordinateEntryInteger> entries = m.coordinate_entries_integer();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    case Field::pattern:
        {
            std::vector<CoordinateEntryPattern> entries = m.coordinate_entries_pattern();
            std::sort(std::begin(entries), std::end(entries), compare_entries);
            return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
        }
    }
    throw matrix_market_error{"Unknown field"};
}

}
