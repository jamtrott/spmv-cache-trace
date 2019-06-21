#include "util/json.h"
#include <gtest/gtest.h>

TEST(json, null_type)
{
    struct json * x = json_null();
    ASSERT_TRUE(json_is_null(x));
    ASSERT_FALSE(json_is_true(x));
    ASSERT_FALSE(json_is_false(x));
    json_free(x);
}

TEST(json, true_type)
{
    struct json * x = json_true();
    ASSERT_FALSE(json_is_null(x));
    ASSERT_TRUE(json_is_true(x));
    ASSERT_FALSE(json_is_false(x));
    json_free(x);
}

TEST(json, false_type)
{
    struct json * x = json_false();
    ASSERT_FALSE(json_is_null(x));
    ASSERT_FALSE(json_is_true(x));
    ASSERT_TRUE(json_is_false(x));
    json_free(x);
}

TEST(json, number_type)
{
    struct json * x = json_number(0);
    ASSERT_FALSE(json_is_null(x));
    ASSERT_FALSE(json_is_true(x));
    ASSERT_FALSE(json_is_false(x));
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(0, json_to_int(x));
    json_free(x);

    x = json_number(1.2);
    ASSERT_EQ(1.2, json_to_double(x));
    json_free(x);
}

TEST(json, string_type)
{
    struct json * x;
    x = json_string("");
    ASSERT_FALSE(json_is_null(x));
    ASSERT_FALSE(json_is_true(x));
    ASSERT_FALSE(json_is_false(x));
    ASSERT_FALSE(json_is_number(x));
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("", json_to_string(x));
    json_free(x);

    x = json_string("abcdefg");
    ASSERT_STREQ("abcdefg", json_to_string(x));
    json_free(x);
}

TEST(json, array_type)
{
    {
        struct json * array[] = {};
        struct json * x = json_array(array, sizeof(array) / sizeof(*array));
        ASSERT_TRUE(json_is_array(x));
        ASSERT_EQ(0, json_array_size(x));
        ASSERT_EQ(json_array_begin(x), json_array_end());
        json_free(x);
    }

    {
        struct json * array[] = {json_number(1), json_number(2)};
        struct json * x = json_array(array, sizeof(array) / sizeof(*array));
        ASSERT_TRUE(json_is_array(x));
        ASSERT_EQ(2, json_array_size(x));
        ASSERT_NE(json_array_begin(x), json_array_end());
        ASSERT_EQ(1, json_to_int(json_array_begin(x)));
        ASSERT_NE(json_array_next(json_array_begin(x)), json_array_end());
        ASSERT_EQ(2, json_to_int(json_array_next(json_array_begin(x))));
        json_free(x);
    }

    {
        struct json * array[] = {json_number(1), json_number(2), json_number(3)};
        struct json * x = json_array(array, sizeof(array) / sizeof(*array));
        ASSERT_TRUE(json_is_array(x));
        ASSERT_EQ(3, json_array_size(x));
        ASSERT_NE(json_array_begin(x), json_array_end());
        ASSERT_EQ(1, json_to_int(json_array_begin(x)));
        ASSERT_NE(json_array_next(json_array_begin(x)), json_array_end());
        ASSERT_EQ(2, json_to_int(json_array_next(json_array_begin(x))));
        ASSERT_NE(json_array_next(json_array_next(json_array_begin(x))),
                  json_array_end());
        ASSERT_EQ(3, json_to_int(
                      json_array_next(json_array_next(json_array_begin(x)))));
        json_free(x);
    }
}

TEST(json, object_type)
{
    {
        struct json * object[] = {};
        struct json * x = json_object(object, sizeof(object) / sizeof(*object));
        ASSERT_TRUE(json_is_object(x));
        ASSERT_EQ(0, json_object_size(x));
        ASSERT_EQ(json_object_begin(x), json_object_end());
        json_free(x);
    }

    {
        struct json * object[] = {
            json_member("a", json_number(1)),
            json_member("b", json_number(2))};
        struct json * x = json_object(object, sizeof(object) / sizeof(*object));
        ASSERT_TRUE(json_is_object(x));
        ASSERT_EQ(2, json_object_size(x));
        struct json * y = json_object_begin(x);
        ASSERT_NE(json_object_end(), y);
        ASSERT_STREQ("a", json_to_key(y));
        ASSERT_EQ(1, json_to_int(json_to_value(y)));
        y = json_object_next(y);
        ASSERT_NE(json_object_end(), y);
        ASSERT_STREQ("b", json_to_key(y));
        ASSERT_EQ(2, json_to_int(json_to_value(y)));
        y = json_object_next(y);
        ASSERT_EQ(json_object_end(), y);
        json_free(x);
    }
}

/*
 * Parser tests.
 */
TEST(json, parse_null)
{
    struct json * x = json_parse("null");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_null(x));
    json_free(x);
}

TEST(json, parse_true)
{
    struct json * x = json_parse("true");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_true(x));
    json_free(x);
}

TEST(json, parse_false)
{
    struct json * x = json_parse("false");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_false(x));
    json_free(x);
}

TEST(json, parse_whitespace)
{
    struct json * x;
    x = json_parse(" null");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_null(x));
    json_free(x);
    x = json_parse(" null ");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_null(x));
    json_free(x);
    x = json_parse("\tnull\n");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_null(x));
    json_free(x);
}

TEST(json, parse_number)
{
    struct json * x;

    // Integers
    x = json_parse("0");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(0, json_to_int(x));
    json_free(x);
    x = json_parse("1");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(1, json_to_int(x));
    json_free(x);
    x = json_parse("12");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(12, json_to_int(x));
    json_free(x);

    // Negative integers
    x = json_parse("-0");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(0, json_to_int(x));
    json_free(x);
    x = json_parse("-1");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(-1, json_to_int(x));
    json_free(x);
    x = json_parse("-12");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(-12, json_to_int(x));
    json_free(x);

    // Decimals
    x = json_parse("1.1");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(1.1, json_to_double(x));
    json_free(x);
    x = json_parse("-1.1");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(-1.1, json_to_double(x));
    json_free(x);

    // Exponents
    x = json_parse("1.1e0");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(1.1, json_to_double(x));
    json_free(x);
    x = json_parse("-1.1e0");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(-1.1, json_to_double(x));
    json_free(x);
    x = json_parse("1.1e1");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(11., json_to_double(x));
    json_free(x);
    x = json_parse("1.1e3");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_number(x));
    ASSERT_EQ(1100, json_to_double(x));
    json_free(x);
}

TEST(json, parse_string)
{
    struct json * x;
    x = json_parse("\"\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("", json_to_string(x));
    json_free(x);
    x = json_parse("\"abc\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("abc", json_to_string(x));
    json_free(x);

    // Escape sequences
    x = json_parse("\"\\\"\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\\"", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\\\\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\\\", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\/\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\/", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\b\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\b", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\f\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\f", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\n\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\n", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\r\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\r", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\t\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\t", json_to_string(x));
    json_free(x);
    x = json_parse("\"\\u00f8\"");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_string(x));
    ASSERT_STREQ("\\u00f8", json_to_string(x));
    json_free(x);
}

TEST(json, parse_array)
{
    struct json * x;
    x = json_parse("[]");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_array(x));
    ASSERT_EQ(0, json_array_size(x));
    json_free(x);
    x = json_parse("[ ]");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_array(x));
    ASSERT_EQ(0, json_array_size(x));
    json_free(x);
    x = json_parse("[1]");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_array(x));
    ASSERT_EQ(1, json_array_size(x));
    ASSERT_NE(nullptr, json_array_get(x, 0));
    ASSERT_EQ(1, json_to_int(json_array_get(x, 0)));
    json_free(x);
    x = json_parse("[1,2]");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_array(x));
    ASSERT_EQ(2, json_array_size(x));
    ASSERT_NE(nullptr, json_array_get(x, 0));
    ASSERT_EQ(1, json_to_int(json_array_get(x, 0)));
    ASSERT_NE(nullptr, json_array_get(x, 1));
    ASSERT_EQ(2, json_to_int(json_array_get(x, 1)));
    json_free(x);
    x = json_parse("[1,2,3]");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_array(x));
    ASSERT_EQ(3, json_array_size(x));
    ASSERT_NE(nullptr, json_array_get(x, 0));
    ASSERT_EQ(1, json_to_int(json_array_get(x, 0)));
    ASSERT_NE(nullptr, json_array_get(x, 1));
    ASSERT_EQ(2, json_to_int(json_array_get(x, 1)));
    ASSERT_NE(nullptr, json_array_get(x, 2));
    ASSERT_EQ(3, json_to_int(json_array_get(x, 2)));
    json_free(x);
}

TEST(json, parse_object)
{
    struct json * x;
    struct json * y;

    x = json_parse("{}");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_object(x));
    ASSERT_EQ(0, json_object_size(x));
    json_free(x);

    x = json_parse("{ }");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_object(x));
    ASSERT_EQ(0, json_object_size(x));
    json_free(x);

    x = json_parse("{\"a\": 1}");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_object(x));
    ASSERT_EQ(1, json_object_size(x));
    ASSERT_NE(nullptr, json_object_get(x, "a"));
    ASSERT_EQ(1, json_to_int(json_object_get(x, "a")));
    y = json_object_begin(x);
    ASSERT_NE(json_object_end(), y);
    ASSERT_STREQ("a", json_to_key(y));
    ASSERT_EQ(1, json_to_int(json_to_value(y)));
    json_free(x);

    x = json_parse("{\"a\": 1, \"b\": 2}");
    ASSERT_FALSE(x && json_is_invalid(x)) << json_to_error(x);
    ASSERT_TRUE(json_is_object(x));
    ASSERT_EQ(2, json_object_size(x));
    ASSERT_NE(nullptr, json_object_get(x, "a"));
    ASSERT_EQ(1, json_to_int(json_object_get(x, "a")));
    ASSERT_NE(nullptr, json_object_get(x, "b"));
    ASSERT_EQ(2, json_to_int(json_object_get(x, "b")));
    y = json_object_begin(x);
    ASSERT_NE(json_object_end(), y);
    ASSERT_STREQ("a", json_to_key(y));
    ASSERT_EQ(1, json_to_int(json_to_value(y)));
    y = json_object_next(y);
    ASSERT_NE(json_object_end(), y);
    ASSERT_STREQ("b", json_to_key(y));
    ASSERT_EQ(2, json_to_int(json_to_value(y)));
    y = json_object_next(y);
    ASSERT_EQ(json_object_end(), y);
    json_free(x);
}
