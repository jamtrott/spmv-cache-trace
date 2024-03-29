#include "matrix-market.hpp"
#include "matrix-market-reorder.hpp"
#include "matrix-error.hpp"
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
#include <cstdio>

#include <cfloat>
#include <cmath>
#include <cstring>
#include <errno.h>

using namespace matrix_market;
using namespace std::literals::string_literals;

namespace matrix_market
{

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

void Matrix::permute (std::vector<int> const & new_order)
{
  if (format() != Format::coordinate) {
    std::cerr << "Expected matrix in coordinate format!\n";
    std::cerr << "No permutation is done.\n";
    return;
  }

  if (field() != Field::real) {
    std::cerr << "Expected matrix with real values!\n";
    std::cerr << "No permutation is done.\n";
    return;
  }

  if (new_order.size() != size().rows || new_order.size() != size().columns) {
    std::cerr << "The dimension of the matrix doesn't match!\n";
    std::cerr << "No permutation is done.\n";
    return;
  }

  for (int e=0; e<size().num_entries; e++) {
    entries_real[e].i = new_order[entries_real[e].i-1]+1;
    entries_real[e].j = new_order[entries_real[e].j-1]+1;
  }
}
}

namespace
{

char asciitolower(char c)
{
    if (c <= 'Z' && c >= 'A')
        return c - ('Z' - 'z');
    return c;
}

Object readObject(std::istream & i)
{
    std::string object;
    i >> object;
    std::transform(object.begin(), object.end(), object.begin(), asciitolower);
    if (object != std::string{"matrix"}) {
        auto s = std::stringstream{};
        s << "Failed to parse header: "
          << "Expected \"matrix\", got \"" << object << "\"";
        throw matrix::matrix_error{s.str()};
    }

    return Object::matrix;
}

Format readFormat(std::istream & i)
{
    std::string format;
    i >> format;
    std::transform(format.begin(), format.end(), format.begin(), asciitolower);
    if (format == std::string{"coordinate"}) {
        return Format::coordinate;
    } else if (format == std::string{"array"}) {
        return Format::array;
    }
    auto s = std::stringstream{};
    s << "Expected \"coordinate\" or \"array\", got \"" << format << "\"";
    throw matrix::matrix_error{s.str()};
}

Field readField(std::istream & i)
{
    std::string field;
    i >> field;
    std::transform(field.begin(), field.end(), field.begin(), asciitolower);
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
    throw matrix::matrix_error{s.str()};
}

Symmetry readSymmetry(std::istream & i)
{
    std::string symmetry;
    i >> symmetry;
    std::transform(symmetry.begin(), symmetry.end(), symmetry.begin(), asciitolower);
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
    throw matrix::matrix_error{s.str()};
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
        throw matrix::matrix_error{s.str()};
    }

    auto object = readObject(s);
    auto format = readFormat(s);
    auto field = readField(s);
    auto symmetry = readSymmetry(s);
    return Header{object, format, field, symmetry};
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
        throw matrix::matrix_error{"Failed to parse size"};

    std::stringstream s{line};
    index_type rows, columns;
    s >> rows;
    if (!s) {
        throw matrix::matrix_error(
            "Failed to parse size: "
            "Integer overflow when reading number of rows");
    }
    s >> columns;
    if (!s) {
        throw matrix::matrix_error(
            "Failed to parse size: "
            "Integer overflow when reading number of columns");
    }

    if (format == Format::array)
        return Size{rows, columns, 0u};

    size_type num_entries;
    s >> num_entries;
    if (!s) {
        throw matrix::matrix_error(
            "Failed to parse size: "
            "Integer overflow when reading number of non-zeros");
    }

    return Size{rows, columns, num_entries};
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
        throw matrix::matrix_error{s.str()};
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
    throw matrix::matrix_error{"Unknown field"};
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
    throw matrix::matrix_error{"Unknown field"};
}

std::string toString(Symmetry const & s)
{
    switch (s) {
    case Symmetry::general: return std::string("generall");
    case Symmetry::symmetric: return std::string("symmetric");
    case Symmetry::skew_symmetric: return std::string("skew-symmetric");
    case Symmetry::hermitian: return std::string("hermitian");
    }
    throw matrix::matrix_error{"Unknown symmetry"};
}

namespace matrix_market
{

bool operator==(Header const & A, Header const & B)
{
    return
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
    return o << "%%MatrixMarket" << ' '
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

matrix_market::Matrix load_gz_matrix(
    std::ifstream & f,
    std::string const & path,
    std::string const & extension,
    std::ostream & o,
    bool verbose)
{
    try {
        zlib::izlibstream gzstream(f.rdbuf());
        return matrix_market::fromStream(gzstream);
    } catch (zlib::zlibstream_error const & e) {
        throw matrix::matrix_error(e.what());
    }
}

matrix_market::Matrix load_targz_matrix(
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
        throw matrix::matrix_error(e.what());
    }
}

matrix_market::Matrix load_matrix(
    std::string const & path,
    std::ostream & o,
    bool verbose)
{
  bool reorder_RCM = false, reorder_GP = false;
  int nparts = 0;
  size_t pos;

  std::string path_ = path;
  pos = path_.rfind("__RCM");
  if (pos<path_.length()) {
    reorder_RCM = true;
    path_.erase(pos, path_.length());
  }

  pos = path_.rfind("__GP");
  if (pos<path_.length()) {
    reorder_GP = true;
    if (pos<path_.length()-4) {
      char tmp[20];
      path_.copy (tmp, path_.length()-pos-4, pos+4);
      std::sscanf (tmp, "%d", &nparts);
    }
    path_.erase(pos, path_.length());
  }

    if (verbose) {
        o << "Loading matrix from " << path_ << '\n';
        if (reorder_RCM)
          o << "The input matrix will be reordered using reverse Cuthill-McKee\n";
        if (reorder_GP)
          o << "The input matrix will be reordered using graph partitioning\n";
    }

    auto f = std::ifstream{path_};
    if (!f)
        throw matrix::matrix_error(strerror(errno));

    if (ends_with(path_, ".tar.gz"s)) {
        matrix_market::Matrix m=load_targz_matrix(f, path_, ".tar.gz"s, o, verbose);
        if (reorder_RCM) {
          std::vector<int> new_order = find_new_order_RCM (m);
          m.permute (new_order);
        }
        if (reorder_GP) {
          std::vector<int> new_order = find_new_order_GP (m, nparts);
          m.permute (new_order);
        }
        return m;
    } else if (ends_with(path_, ".tgz"s)) {
        matrix_market::Matrix m=load_targz_matrix(f, path_, ".tgz"s, o, verbose);
        if (reorder_RCM) {
          std::vector<int> new_order = find_new_order_RCM (m);
          m.permute (new_order);
        }
        if (reorder_GP) {
          std::vector<int> new_order = find_new_order_GP (m, nparts);
          m.permute (new_order);
        }
        return m;
    } else if (ends_with(path_, ".gz"s)) {
        matrix_market::Matrix m=load_gz_matrix(f, path_, ".gz"s, o, verbose);
        if (reorder_RCM) {
          std::vector<int> new_order = find_new_order_RCM (m);
          m.permute (new_order);
        }
        if (reorder_GP) {
          std::vector<int> new_order = find_new_order_GP (m, nparts);
          m.permute (new_order);
        }
        return m;
    } else {
        matrix_market::Matrix m=matrix_market::fromStream(f);
        if (reorder_RCM) {
          std::vector<int> new_order = find_new_order_RCM (m);
          m.permute (new_order);
        }
        if (reorder_GP) {
          std::vector<int> new_order = find_new_order_GP (m, nparts);
          m.permute (new_order);
        }
        return m;
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
    throw matrix::matrix_error{"Unknown field"};
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
    throw matrix::matrix_error{"Unknown field"};
}

}
