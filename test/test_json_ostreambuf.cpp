#include "util/json-ostreambuf.hpp"

#include <limits>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

using namespace std::literals::string_literals;

TEST(json_ostreambuf, null)
{
    std::stringstream stream;
    auto o = json_ostreambuf(stream);
    ASSERT_EQ(""s, stream.str());
}

TEST(json_ostreambuf, object)
{
    std::stringstream stream;
    auto o = json_ostreambuf(stream);
    stream << '{' << '\n'
           << '"' << "a" << '"' << ": " << 1 << ',' << '\n'
           << '"' << "b" << '"' << ": " << 2 << '\n'
           << '}';

    std::string expected =
        "{\n"
        "  \"a\": 1,\n"
        "  \"b\": 2\n"
        "}";
    ASSERT_EQ(expected, stream.str());
}

TEST(json_ostreambuf, array)
{
    std::stringstream stream;
    auto o = json_ostreambuf(stream);
    stream << '[' << '\n'
           << 1 << ',' << '\n'
           << 2 << '\n'
           << ']';

    std::string expected =
        "[\n"
        "  1,\n"
        "  2\n"
        "]";
    ASSERT_EQ(expected, stream.str());
}

TEST(json_ostreambuf, nan)
{
    std::stringstream stream;
    auto o = json_ostreambuf(stream);
    stream << std::numeric_limits<double>::quiet_NaN();
    std::string expected = "\"nan\"";
    ASSERT_EQ(expected, stream.str());
}
