#ifndef ZLIBSTREAM_HPP
#define ZLIBSTREAM_HPP

#include <zlib.h>

#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

std::ostream & operator<<(std::ostream & o, z_stream const & zs);

namespace zlib
{

class zlibstreambuf
    : public std::streambuf
{
public:
    zlibstreambuf(
        std::streambuf * sbuf,
        std::streamsize buf_size = 128u*1024u);
    ~zlibstreambuf();

    zlibstreambuf(zlibstreambuf const &) = delete;
    zlibstreambuf(zlibstreambuf &&) = delete;
    zlibstreambuf & operator=(zlibstreambuf const &) = delete;
    zlibstreambuf & operator=(zlibstreambuf &&) = delete;

    int underflow() override;

private:
    std::streambuf * sbuf_;
    std::vector<Bytef> in_buf;
    Bytef * in_buf_it;
    Bytef * in_buf_end;
    std::vector<Bytef> out_buf;
    Bytef * out_buf_end;
    z_stream zs;
    bool stream_initialized;
};

class zlibstream_base
{
public:
    zlibstream_base(std::streambuf * sbuf);
protected:
    zlibstreambuf sbuf_;
};

class izlibstream
    : virtual zlibstream_base
    , public std::istream
{
public:
    izlibstream(std::streambuf * sbuf);
};

struct zlibstream_error
    : public std::system_error
{
    zlibstream_error(int ev, const std::string & what_arg);
    zlibstream_error(int ev, const char * what_arg);
};

}

#endif
