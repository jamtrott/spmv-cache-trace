#ifndef ZLIBSTREAM_HPP
#define ZLIBSTREAM_HPP

#include <zlib.h>

#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace zlib
{

struct error_category : std::error_category
{
    char const * name() const noexcept override
    {
        return "zlib::error_category";
    }

    std::string message(int ev) const override
    {
        return std::string(zError(ev));
    }
};

}

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

struct zlibstream_base
{
    zlibstreambuf sbuf_;
    zlibstream_base(std::streambuf * sbuf) : sbuf_(sbuf) {}
};

class izlibstream
    : virtual zlibstream_base
    , public std::istream
{
public:
    izlibstream(std::streambuf * sbuf)
        : zlibstream_base(sbuf)
        , std::ios(&this->sbuf_)
        , std::istream(&this->sbuf_)
    {
    }
};

}

#endif
