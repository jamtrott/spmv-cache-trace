#ifndef JSON_OSTREAMBUF_HPP
#define JSON_OSTREAMBUF_HPP

#include <iosfwd>
#include <locale>
#include <streambuf>

template <typename Iterator = std::ostreambuf_iterator<char>>
class json_num_put
    : public std::num_put<char, Iterator>
{
private:
    using base_type = std::num_put<char, Iterator>;

public:
    using char_type = typename base_type::char_type;
    using iter_type = typename base_type::iter_type;

public:
    json_num_put(std::size_t refs = 0);

protected:
    virtual iter_type do_put(
        iter_type out,
        std::ios_base & s,
        char_type fill,
        double x) const override;

    virtual iter_type do_put(
        iter_type out,
        std::ios_base & s,
        char_type fill,
        long double x) const override;
};

class json_ostreambuf
    : public std::streambuf
{
public:
    explicit json_ostreambuf(
        std::streambuf * sb);
    explicit json_ostreambuf(
        std::ostream & o_);
    virtual ~json_ostreambuf();

    json_ostreambuf(json_ostreambuf const &);
    json_ostreambuf & operator=(json_ostreambuf const &);

protected:
    virtual int overflow(int ch);

private:
    std::ostream * o;
    std::streambuf * sb;
    json_num_put<> num_put;
    std::locale locale;
    bool start_of_line;
    int indent;
};

#endif
