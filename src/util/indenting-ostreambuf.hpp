#ifndef INDENTING_OSTREAMBUF_HPP
#define INDENTING_OSTREAMBUF_HPP

#include <iosfwd>
#include <streambuf>

class indenting_ostreambuf
    : public std::streambuf
{
public:
    explicit indenting_ostreambuf(
        std::streambuf * sb);
    explicit indenting_ostreambuf(
        std::streambuf * sb,
        int indent);
    explicit indenting_ostreambuf(
        std::ostream & o_);
    explicit indenting_ostreambuf(
        std::ostream & o_,
        int indent);
    virtual ~indenting_ostreambuf();

    void set_indent(int indent);

protected:
    virtual int overflow(int ch);

private:
    std::ostream * o;
    std::streambuf * sb;
    bool start_of_line;
    int indent;
};

#endif
