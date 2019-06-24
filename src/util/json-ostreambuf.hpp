#ifndef JSON_OSTREAMBUF_HPP
#define JSON_OSTREAMBUF_HPP

#include <iostream>

#include <ostream>
#include <streambuf>
#include <string>

class json_ostreambuf
    : public std::streambuf
{
public:
    explicit json_ostreambuf(
        std::streambuf * sb)
        : o(nullptr)
        , sb(sb)
        , start_of_line(true)
        , indent(0)
    {
    }

    explicit json_ostreambuf(
        std::ostream & o_)
        : o(&o_)
        , sb(o->rdbuf())
        , start_of_line(true)
        , indent(0)
    {
        o->rdbuf(this);
    }

    virtual ~json_ostreambuf()
    {
        if (o != nullptr)
            o->rdbuf(sb);
    }

protected:
    virtual int overflow(int ch) {
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

private:
    std::ostream * o;
    std::streambuf * sb;
    bool start_of_line;
    int indent;
};

#endif
