#include "zlibstream.hpp"

#include <zlib.h>

#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <system_error>

std::ostream & operator<<(std::ostream & o, z_stream const & zs)
{
    return o << "z_stream{"
             << &zs.next_in << ", "
             << zs.avail_in << ", "
             << zs.total_in << ", "
             << &zs.next_out << ", "
             << zs.avail_out << ", "
             << zs.total_out << '}';
}

namespace zlib
{

zlibstreambuf::zlibstreambuf(
    std::streambuf * sbuf,
    std::streamsize buf_size)
    : sbuf_(sbuf)
    , in_buf(buf_size)
    , in_buf_it(nullptr)
    , in_buf_end(nullptr)
    , out_buf(buf_size)
    , out_buf_end(out_buf.data())
    , stream_initialized(false)
{
}

zlibstreambuf::~zlibstreambuf()
{
    if (stream_initialized)
        inflateEnd(&zs);
}

int zlibstreambuf::underflow()
{
    if (this->gptr() == this->egptr()) {
        // Decompress data from the input stream buffer
        out_buf_end = out_buf.data();
        do {
            if (in_buf_it == in_buf_end) {
                auto bytes_read = sbuf_->sgetn((char *)in_buf.data(), in_buf.size());
                if (bytes_read == 0u) {
                    break;
                }
                in_buf_it = in_buf.data();
                in_buf_end = in_buf.data() + bytes_read;
            }

            if (!stream_initialized) {
                memset(&zs, 0, sizeof(zs));
                zs.zalloc = Z_NULL;
                zs.zfree = Z_NULL;
                zs.opaque = Z_NULL;
                zs.avail_in = 0;
                zs.next_in = Z_NULL;
                auto err = inflateInit2(&zs, 15+32);
                if (err != Z_OK) {
                    auto s = std::stringstream{};
                    s << "inflateInit2(" << zs << ", 15+32)";
                    throw std::system_error(err, zlib::error_category(), s.str());
                }
                stream_initialized = true;
            }

            zs.next_in = in_buf_it;
            zs.avail_in = in_buf_end - in_buf_it;
            zs.next_out = out_buf_end;
            zs.avail_out = (out_buf.data() + out_buf.size()) - out_buf_end;
            auto err = inflate(&zs, Z_NO_FLUSH);
            if (err != Z_OK && err != Z_STREAM_END) {
                auto s = std::stringstream{};
                s << "inflate(" << zs << ", Z_NO_FLUSH)";
                throw std::system_error(err, zlib::error_category(), s.str());
            }

            in_buf_it = zs.next_in;
            in_buf_end = zs.next_in + zs.avail_in;
            out_buf_end = zs.next_out;
        } while (out_buf_end == out_buf.data());

        this->setg((char *)out_buf.data(), (char *)out_buf.data(), (char *)out_buf_end);
    }

    return this->gptr() == this->egptr()
        ? std::char_traits<char>::eof()
        : std::char_traits<char>::to_int_type(*this->gptr());
}

}
