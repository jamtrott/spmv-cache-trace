#ifndef TARSTREAM_HPP
#define TARSTREAM_HPP

#include <iosfwd>
#include <istream>
#include <streambuf>
#include <string>
#include <vector>

namespace tar
{

struct tar_header
{
    char filename[100];
    char mode[8];
    char owner_id[8];
    char group_id[8];
    char size[12]; /* File size in bytes (octal base) */
    char last_modification_time[12]; /* Last modification time in
                                      * numeric Unix time format (octal) */
    char checksum[8];
    char type;
    char link_name[100];
    char ustar_indicator[6];
    char ustar_version[2];
    char owner_username[32];
    char owner_groupname[32];
    char device_major_number[8];
    char device_minor_number[8];
    char filename_prefix[155];
    char padding[12];
};

/*
 * In a tarball, locate a TAR file header corresponding to a given file.
 */
tar_header find_tar_header(
    std::streambuf & sbuf,
    std::vector<char> & buf,
    std::string const & filename,
    bool & header_found);

class tarstreambuf
    : public std::streambuf
{
public:
    tarstreambuf(
        std::streambuf * sbuf,
        std::string const & filename);
    tarstreambuf(tarstreambuf const &) = delete;
    tarstreambuf(tarstreambuf &&) = delete;
    tarstreambuf & operator=(tarstreambuf const &) = delete;
    tarstreambuf & operator=(tarstreambuf &&) = delete;

    int underflow() override;

private:
    std::streambuf * sbuf_;
    std::string filename;
    tar_header header;
    bool header_found;
    std::streamsize bytes_remaining;
    std::vector<char> buf;
};

struct tarstream_base
{
    tarstreambuf sbuf_;
    tarstream_base(
        std::streambuf * sbuf,
        std::string const & filename)
        : sbuf_(sbuf, filename)
    {
    }
};

class itarstream
    : virtual tarstream_base
    , public std::istream
{
public:
    itarstream(
        std::streambuf * sbuf,
        std::string const & filename)
        : tarstream_base(sbuf, filename)
        , std::ios(&this->sbuf_)
        , std::istream(&this->sbuf_)
    {
    }
};

}

#endif
