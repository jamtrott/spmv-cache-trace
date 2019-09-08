#include "json-ostreambuf.hpp"

#include <cmath>
#include <locale>
#include <ostream>
#include <streambuf>
#include <string>

template <typename Iterator>
json_num_put<Iterator>::json_num_put(std::size_t refs)
    : base_type(refs)
{
}

template <typename Iterator>
typename json_num_put<Iterator>::iter_type json_num_put<Iterator>::do_put(
    iter_type out,
    std::ios_base & s,
    char_type fill,
    double x) const
{
    std::string nan("\"nan\"");
    if (std::isnan(x))
        out = std::copy(std::begin(nan), std::end(nan), out);
    else
        out = base_type::do_put(out, s, fill, x);
    return out;
}

template <typename Iterator>
typename json_num_put<Iterator>::iter_type json_num_put<Iterator>::do_put(
    iter_type out,
    std::ios_base & s,
    char_type fill,
    long double x) const
{
    std::string nan("\"nan\"");
    if (std::isnan(x))
        out = std::copy(std::begin(nan), std::end(nan), out);
    else
        out = base_type::do_put(out, s, fill, x);
    return out;
}

json_ostreambuf::json_ostreambuf(
    std::streambuf * sb)
    : o(nullptr)
    , sb(sb)
    , num_put(1)
    , locale(sb->getloc(), &num_put)
    , start_of_line(true)
    , indent(0)
{
    sb->pubimbue(locale);
}

json_ostreambuf::json_ostreambuf(
    std::ostream & o_)
    : o(&o_)
    , sb(o->rdbuf())
    , num_put(1)
    , locale(o->getloc(), &num_put)
    , start_of_line(true)
    , indent(0)
{
    o->rdbuf(this);
    o->imbue(locale);
}

json_ostreambuf::~json_ostreambuf()
{
    if (o != nullptr)
        o->rdbuf(sb);
}

int json_ostreambuf::overflow(int ch) {
    if (ch == '}' || ch == ']')
        indent -= 2;

    if (start_of_line && ch != '\n') {
        auto indent_s = std::string((indent > 0) ? indent : 0, ' ');
        sb->sputn(indent_s.data(), indent_s.size());
    }
    start_of_line = (ch == '\n');

    if (ch == '{' || ch == '[')
        indent += 2;
    return sb->sputc(ch);
}
