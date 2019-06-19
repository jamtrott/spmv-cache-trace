#include "tarstream.hpp"

#include <cstring>
#include <ostream>
#include <string>

namespace tar
{

/*
 * Decode an octal value stored as ASCII text.
 */
uint64_t decode_octal(char * s, std::size_t size)
{
    // Remove leading null or whitespace characters
    while (size > 0u && (*s == '\0' || *s == ' ')) {
        s++;
        size--;
    }

    if (size == 0u || s[0] == '\0' || s[0] == ' ')
        return 0u;

    // Convert octal to decimal
    uint64_t x = s[0u] - 48;
    for (auto i = 1u; i < size; ++i) {
        if (s[i] == '\0' || s[i] == ' ')
            break;
        x = (s[i] - 48) + x * 8u;
    }
    return x;
}

uint64_t decode_binary(char * s, std::size_t size)
{
    uint64_t x = 0u;
    for (auto i = 0u; i < size; ++i)
        x = s[i] + x * 256u;
    return x;
}

uint64_t decode_numeric_field(char * s, std::size_t size)
{
    if (size == 0u)
        return 0u;

    if (s[0] & 0x80) {
        s[0] &= ~0x80;
        return decode_binary(s, size);
    } else {
        return decode_octal(s, size);
    }
}

/*
 * In a tarball, locate a TAR file header corresponding to a given file.
 */
tar_header find_tar_header(
    std::streambuf & sbuf,
    std::vector<char> & buf,
    std::string const & filename,
    bool & header_found)
{
    bool first_end_block = false;
    while (true) {
        // Look for the filename in the next TAR file header
        tar_header h;
        auto header_bytes_read = sbuf.sgetn(
            reinterpret_cast<char *>(&h), sizeof(h));
        if (header_bytes_read != sizeof(h)) {
            header_found = false;
            return tar_header{};
        }
        auto len = std::min(filename.size(), sizeof(h.filename));
        if (strncmp(filename.c_str(), h.filename, len) == 0) {
            header_found = true;
            return h;
        }

        // Look for the the end-of-archive indicator, two blocks of
        // all zeros.
        auto size = decode_numeric_field(h.size, sizeof(h.size));
        if (size == 0u) {
            tar_header zero_header;
            memset(&zero_header, 0, sizeof(zero_header));
            if (memcmp(&h, &zero_header, sizeof(h)) == 0) {
                if (first_end_block)
                    break;
                first_end_block = true;
            } else {
                first_end_block = false;
            }
        } else {
            first_end_block = false;
        }

        // Skip this file by reading 512-byte blocks until we get to
        // the next file header.
        auto min_file_size = 512u;
        decltype(buf.size()) bytes_remaining =
            ((size + (min_file_size - 1u)) / min_file_size) * min_file_size;
        while (bytes_remaining > 0) {
            auto bytes_available = sbuf.sgetn(
                buf.data(), std::min(bytes_remaining, buf.size()));
            if (bytes_available == 0u) {
                header_found = false;
                return tar_header{};
            }
            bytes_remaining -= bytes_available;
        }
    }
    header_found = false;
    return tar_header{};
}

tarstreambuf::tarstreambuf(
    std::streambuf * sbuf,
    std::string const & filename)
    : sbuf_(sbuf)
    , filename(filename)
    , header()
    , header_found(false)
    , bytes_remaining(0u)
    , buf(8*1024u)
{
}

int tarstreambuf::underflow()
{
    // Read data from the tar archive
    if (this->gptr() == this->egptr()) {

        // Locate the header of the file that we want to read
        if (!header_found) {
            header = find_tar_header(*sbuf_, buf, filename, header_found);
            if (!header_found) {
                return this->gptr() == this->egptr()
                    ? std::char_traits<char>::eof()
                    : std::char_traits<char>::to_int_type(*this->gptr());
            }
            auto & file_size = header.size;
            bytes_remaining = decode_numeric_field(file_size, sizeof(file_size));
        }

        // Read a block of data from the tar file
        if (header_found) {
            auto bytes_available = sbuf_->sgetn(buf.data(), buf.size());
            if (bytes_available > 0u && bytes_remaining > 0u) {
                bytes_available = std::min(bytes_available, bytes_remaining);
                this->setg(buf.data(), buf.data(), buf.data() + bytes_available);
                bytes_remaining -= bytes_available;
            }
        }
    }

    return this->gptr() == this->egptr()
        ? std::char_traits<char>::eof()
        : std::char_traits<char>::to_int_type(*this->gptr());
}

}
