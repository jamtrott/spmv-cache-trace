#ifndef JSON_OSTREAMBUF_HPP
#define JSON_OSTREAMBUF_HPP

#include <iosfwd>
#include <streambuf>

class json_ostreambuf
    : public std::streambuf
{
public:
    explicit json_ostreambuf(
        std::streambuf * sb);
    explicit json_ostreambuf(
        std::ostream & o_);
    virtual ~json_ostreambuf();

protected:
    virtual int overflow(int ch);

private:
    std::ostream * o;
    std::streambuf * sb;
    bool start_of_line;
    int indent;
};

#endif
