#include "indenting-ostreambuf.hpp"

#include <ostream>
#include <streambuf>
#include <string>

indenting_ostreambuf::indenting_ostreambuf(
    std::streambuf * sb_)
    : o(nullptr)
    , sb(sb_)
    , start_of_line(true)
    , indent(0)
{
}

indenting_ostreambuf::indenting_ostreambuf(
    std::streambuf * sb_, int indent_)
    : o(nullptr)
    , sb(sb_)
    , start_of_line(true)
    , indent(indent_)
{
}

indenting_ostreambuf::indenting_ostreambuf(
    std::ostream & o_)
    : o(&o_)
    , sb(o->rdbuf())
    , start_of_line(true)
    , indent(0)
{
    o->rdbuf(this);
}

indenting_ostreambuf::indenting_ostreambuf(
    std::ostream & o_, int indent_)
    : o(&o_)
    , sb(o->rdbuf())
    , start_of_line(true)
    , indent(indent_)
{
    o->rdbuf(this);
}

indenting_ostreambuf::~indenting_ostreambuf()
{
    if (o != nullptr)
        o->rdbuf(sb);
}

int indenting_ostreambuf::overflow(int ch) {
    if (start_of_line && ch != '\n') {
        auto indent_s = std::string((indent > 0) ? indent : 0, ' ');
        sb->sputn(indent_s.data(), indent_s.size());
    }
    start_of_line = (ch == '\n');
    return sb->sputc(ch);
}

void indenting_ostreambuf::set_indent(int indent_level)
{
    indent = indent_level;
}
