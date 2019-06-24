#include "util/json.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * JSON element types.
 */
enum json_type
{
    invalid_t,
    null_t,
    true_t,
    false_t,
    number_t,
    string_t,
    array_t,
    object_t,
    member_t,
};

/*
 * JSON type constructors.
 */
struct json * json_element(void)
{
    struct json * x = (struct json *) malloc(sizeof(struct json));
    if (!x)
        return NULL;
    x->type = invalid_t;
    x->number = 0.0;
    x->string = NULL;
    x->child = NULL;
    x->next = NULL;
    x->prev = NULL;
    return x;
}

void json_free(struct json * x)
{
    if (x && x->child)
        json_free(x->child);
    if (x && x->next)
        json_free(x->next);
    if (x && (json_is_invalid(x) || json_is_string(x) || json_is_member(x)))
        free((char *) x->string);
    free(x);
}

struct json * json_null(void)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = null_t;
    return x;
}

struct json * json_true(void)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = true_t;
    return x;
}

struct json * json_false(void)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = false_t;
    return x;
}

struct json * json_number(double n)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = number_t;
    x->number = n;
    return x;
}

struct json * json_string(const char * s)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = string_t;
    x->string = strdup(s);
    return x;
}

struct json * json_array(struct json ** elements, json_size_type n)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    for (json_size_type i = 0; i < n-1; i++)
        elements[i]->next = elements[i+1];
    for (json_size_type i = 1; i < n; i++)
        elements[i]->prev = elements[i-1];
    x->type = array_t;
    x->child = (n > 0) ? elements[0] : NULL;
    return x;
}

struct json * json_object(struct json ** elements, json_size_type n)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    for (json_size_type i = 0; i < n-1; i++)
        elements[i]->next = elements[i+1];
    for (json_size_type i = 1; i < n; i++)
        elements[i]->prev = elements[i-1];
    x->type = object_t;
    x->child = (n > 0) ? elements[0] : NULL;
    return x;
}

struct json * json_member(const char * key, struct json * value)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    x->type = member_t;
    x->string = strdup(key);
    x->child = value;
    return x;
}

/*
 * Type checks for JSON elements.
 */
bool json_is_invalid(const struct json * x)
{
    return x->type == invalid_t;
}

bool json_is_null(const struct json * x)
{
    return x->type == null_t;
}

bool json_is_true(const struct json * x)
{
    return x->type == true_t;
}

bool json_is_false(const struct json * x)
{
    return x->type == false_t;
}

bool json_is_number(const struct json * x)
{
    return x->type == number_t;
}

bool json_is_string(const struct json * x)
{
    return x->type == string_t;
}

bool json_is_array(const struct json * x)
{
    return x->type == array_t;
}

bool json_is_object(const struct json * x)
{
    return x->type == object_t;
}

bool json_is_member(const struct json * x)
{
    return x->type == member_t;
}

/*
 * Conversion from JSON elements to C data types.
 */
int json_to_int(const struct json * x)
{
    return (int) x->number;
}

double json_to_double(const struct json * x)
{
    return x->number;
}

const char * json_to_string(const struct json * x)
{
    return x->string;
}

const char * json_to_key(const struct json * x)
{
    return x->string;
}

struct json * json_to_value(const struct json * x)
{
    return x->child;
}

const char * json_to_error(const struct json * x)
{
    return x->string;
}

/*
 * JSON arrays.
 */
json_size_type json_array_size(const struct json * x)
{
    json_size_type size = 0;
    const struct json * y;
    for (y = json_array_begin(x); y != json_array_end(); y = json_array_next(y))
        size++;
    return size;
}

struct json * json_array_get(const struct json * x, json_size_type i)
{
    struct json * y = json_array_begin(x);
    for (json_size_type j = 0; j < i && y != json_array_end(); j++)
        y = json_array_next(y);
    return y;
}

struct json * json_array_begin(const struct json * x)
{
    return x->child;
}

struct json * json_array_end(void)
{
    return NULL;
}

struct json * json_array_next(const struct json * x)
{
    return x->next;
}

struct json * json_array_rbegin(const struct json * x)
{
    struct json * y = x->child;
    while (y->next != NULL)
        y = y->next;
    return y;
}

struct json * json_array_rend(void)
{
    return NULL;
}

struct json * json_array_prev(const struct json * x)
{
    return x->prev;
}

/*
 * JSON objects.
 */
json_size_type json_object_size(const struct json * x)
{
    json_size_type size = 0;
    const struct json * y;
    for (y = json_object_begin(x); y != json_object_end(); y = json_object_next(y))
        size++;
    return size;
}

struct json * json_object_get(const struct json * x, const char * key)
{
    struct json * y;
    for (y = json_object_begin(x); y != json_object_end(); y = json_object_next(y)) {
        if (strcmp(key, json_to_key(y)) == 0)
            return json_to_value(y);
    }
    return NULL;
}

struct json * json_object_begin(const struct json * x)
{
    return x->child;
}

struct json * json_object_end(void)
{
    return NULL;
}

struct json * json_object_next(const struct json * x)
{
    return x->next;
}

struct json * json_object_rbegin(const struct json * x)
{
    struct json * y = x->child;
    while (y->next != NULL)
        y = y->next;
    return y;
}

struct json * json_object_rend(void)
{
    return NULL;
}

struct json * json_object_prev(const struct json * x)
{
    return x->prev;
}

/*
 * Helper functions for JSON parser.
 */
bool json_is_whitespace(char c)
{
    return (c == 0x09) || (c == 0x0a) || (c == 0x0d) || (c == 0x20);
}

bool json_is_newline(char c)
{
    return c == 0x0a;
}

bool json_is_digit(const char c)
{
    return (c >= '0' && c <= '9');
}

bool json_is_hex(const char c)
{
    if (c >= '0' && c <= '9')
        return true;
    if (c >= 'a' && c <= 'f')
        return true;
    if (c >= 'A' && c <= 'F')
        return true;
    return false;
}

bool json_is_escape_char(char c)
{
    return (c == '"' || c ==  '\\' || c ==  '/' ||
            c ==  'b' || c ==  'f' || c ==  'n' ||
            c ==  'r' || c ==  't');
}

int json_escape_sequence_len(const char * s)
{
    if (*s == '\\' && json_is_escape_char(*(s+1)))
        return 2;

    if (*s == '\\' &&
        *(s+1) == 'u' &&
        json_is_hex(*(s+2)) &&
        json_is_hex(*(s+3)) &&
        json_is_hex(*(s+4)) &&
        json_is_hex(*(s+5)))
        return 6;

    return -1;
}

/*
 * JSON parser.
 */
struct json_stream
{
    const char * s;
    int line;
    int column;
};

struct json_stream json_stream(const char * s)
{
    struct json_stream stream;
    stream.s = s;
    stream.line = 1;
    stream.column = 1;
    return stream;
}

bool json_stream_is_end_of_stream(struct json_stream stream)
{
    return (*(stream.s)) == '\0';
}

struct json_stream json_stream_advance(
    struct json_stream stream, int n, bool ignore_newlines)
{
    for (int i = 0; i < n; i++) {
        if (json_stream_is_end_of_stream(stream))
            return stream;

        if (!ignore_newlines && json_is_newline(*(stream.s))) {
            stream.line++;
            stream.column = 0;
        } else {
            stream.column++;
        }
        stream.s++;
    }

    return stream;
}

const char * json_parser_error(struct json_stream s, const char * description)
{
    char * error = NULL;
    const char * format = "%d:%d: ";
    int prefix_len = snprintf(error, 0, format, s.line, s.column);
    if (prefix_len < 0) {
        fprintf(stderr, "%d:%d: %s\n", s.line, s.column, description);
        return NULL;
    }

    size_t len = prefix_len + strlen(description);
    error = (char *) malloc(len + 1);
    if (!error) {
        fprintf(stderr, "%d:%d: %s\n", s.line, s.column, description);
        return NULL;
    }

    if (snprintf(error, len, format, s.line, s.column) != prefix_len) {
        fprintf(stderr, "%d:%d: %s\n", s.line, s.column, description);
        return NULL;
    }
    strcat(error + prefix_len, description);
    return error;
}

void make_json_parser_error(
    struct json * x,
    struct json_stream stream,
    const char * description)
{
    if (x->string)
        free((void *) x->string);
    x->type = invalid_t;
    x->string = json_parser_error(stream, description);
}

void copy_json_parser_error(
    struct json * x,
    struct json * y)
{
    if (x->string)
        free((void *) x->string);
    x->type = invalid_t;
    x->string = strdup(json_to_error(y));
}

struct json_stream json_parse_element(struct json * x, struct json_stream stream);

struct json_stream json_parse_whitespace(struct json_stream stream)
{
    while (!json_stream_is_end_of_stream(stream) && json_is_whitespace(*(stream.s))) {
        stream = json_stream_advance(stream, 1, false);
    }
    return stream;
}

struct json_stream json_parse_int(struct json * x, struct json_stream stream)
{
    if (*(stream.s) == '0') {
        x->type = number_t;
        x->number = 0;
        stream = json_stream_advance(stream, 1, false);
        return stream;
    }
    else if (json_is_digit(*(stream.s))) {
        struct json_stream start = stream;
        struct json_stream end = stream;
        while (json_is_digit(*(end.s)))
            end = json_stream_advance(end, 1, false);
        stream = end;
        end.s--;

        double number = 0.0;
        double j = 1;
        while (end.s >= start.s) {
            number += (*(end.s) - '0') * j;
            j = j * 10;
            end.s--;
        }
        x->type = number_t;
        x->number = number;
        return stream;
    }
    else if (*(stream.s) == '-') {
        stream = json_stream_advance(stream, 1, false);
        stream = json_parse_int(x, stream);
        if (x->type == number_t)
            x->number = -x->number;
        return stream;
    }

    make_json_parser_error(
        x, stream, "Ill-formed expression: Expected '0'-'9' or '-'");
    return stream;
}

struct json_stream json_parse_frac(struct json * x, struct json_stream stream)
{
    if (*(stream.s) != '.')
        return stream;
    stream = json_stream_advance(stream, 1, false);

    if (!json_is_digit(*(stream.s))) {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '0'-'9'");
        return stream;
    }

    double number = 0.0;
    double j = 10;
    while (json_is_digit(*(stream.s))) {
        number += (*(stream.s) - '0') / j;
        j = j * 10;
        stream = json_stream_advance(stream, 1, false);
    }
    if (x->number < 0)
        x->number -= number;
    else x->number += number;
    return stream;
}

struct json_stream json_parse_exp(struct json * x, struct json_stream stream)
{
    if (!(*(stream.s) == 'E' || *(stream.s) == 'e'))
        return stream;
    stream = json_stream_advance(stream, 1, false);

    int sign = 1;
    if (*(stream.s) == '+') {
        stream = json_stream_advance(stream, 1, false);
    } else if (*(stream.s) == '-') {
        sign = -1;
        stream = json_stream_advance(stream, 1, false);
    }

    if (!json_is_digit(*(stream.s))) {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '0'-'9'");
        return stream;
    }

    struct json_stream start = stream;
    struct json_stream end = stream;
    while (json_is_digit(*(end.s)))
        end = json_stream_advance(end, 1, false);
    stream = end;
    end.s--;

    int exponent = 0;
    int j = 1;
    while (end.s >= start.s) {
        exponent += (*(end.s) - '0') * j;
        j = j * 10;
        end.s--;
    }

    double factor = 1.0;
    if (sign > 0) {
        for (int i = 0; i < exponent; i++)
            factor = factor * 10;
    } else {
        for (int i = 0; i < exponent; i++)
            factor = factor / 10;
    }

    x->number *= factor;
    return stream;
}

struct json_stream json_parse_number(struct json * x, struct json_stream stream)
{
    stream = json_parse_int(x, stream);
    stream = json_parse_frac(x, stream);
    return json_parse_exp(x, stream);
}

struct json_stream json_parse_string(struct json * x, struct json_stream stream)
{
    struct json_stream start = stream;
    if (*(stream.s) != '"') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '\"'");
        return stream;
    }
    stream = json_stream_advance(stream, 1, true);

    // Read the string, checking for control characters
    // and escape sequences.
    while (*(stream.s) != '"') {
        if (*(stream.s) < 0x001F) {
            make_json_parser_error(
                x, stream,
                "Ill-formed expression: "
                "Control characters are not allowed in strings");
            return stream;
        }
        else if (*(stream.s) == '\\') {
            int len = json_escape_sequence_len(stream.s);
            if (len < 0) {
                make_json_parser_error(
                    x, stream, "Ill-formed expression: Invalid escape sequence");
                return stream;
            }
            stream = json_stream_advance(stream, len, true);
        } else {
            stream = json_stream_advance(stream, 1, true);
        }
    }
    stream = json_stream_advance(stream, 1, false);

    size_t len = (stream.s-1) - (start.s+1);
    char * string = (char *) malloc(len + 1);
    if (!string) {
        make_json_parser_error(x, start, strerror(errno));
        return stream;
    }
    strncpy(string, start.s+1, len);
    string[len] = '\0';
    x->string = string;
    x->type = string_t;
    return stream;
}

struct json_stream json_parse_array(struct json * x, struct json_stream stream)
{
    if (*(stream.s) != '[') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '['");
        return stream;
    }
    stream = json_stream_advance(stream, 1, false);

    stream = json_parse_whitespace(stream);
    if (*(stream.s) == ']') {
        stream = json_stream_advance(stream, 1, false);
        x->child = NULL;
        x->type = array_t;
        return stream;
    }

    struct json * y = json_element();
    stream = json_parse_element(y, stream);
    if (json_is_invalid(y)) {
        copy_json_parser_error(x, y);
        json_free(y);
        return stream;
    }

    struct json * z = y;
    while (*(stream.s) == ',') {
        stream = json_stream_advance(stream, 1, false);
        z->next = json_element();
        if (!z->next) {
            make_json_parser_error(x, stream,  strerror(errno));
            return stream;
        }

        stream = json_parse_element(z->next, stream);
        if (json_is_invalid(z->next)) {
            copy_json_parser_error(x, z->next);
            json_free(y);
            return stream;
        }

        z->next->prev = z;
        z = z->next;
    }
    if (*(stream.s) != ']') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected ']'");
        json_free(y);
        return stream;
    }
    stream = json_stream_advance(stream, 1, false);
    x->child = y;
    x->type = array_t;
    return stream;
}

struct json_stream json_parse_member(struct json * x, struct json_stream stream)
{
    stream = json_parse_whitespace(stream);
    stream = json_parse_string(x, stream);
    if (json_is_invalid(x))
        return stream;
    stream = json_parse_whitespace(stream);
    if (*(stream.s) != ':') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected ':'");
        return stream;
    }
    stream = json_stream_advance(stream, 1, false);

    struct json * y = json_element();
    if (!y) {
        make_json_parser_error(x, stream, strerror(errno));
        return stream;
    }
    stream = json_parse_element(y, stream);
    if (json_is_invalid(y)) {
        copy_json_parser_error(x, y);
        json_free(y);
        return stream;
    }

    x->type = member_t;
    x->child = y;
    return stream;
}

struct json_stream json_parse_object(struct json * x, struct json_stream stream)
{
    if (*(stream.s) != '{') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '{'");
        return stream;
    }
    stream = json_stream_advance(stream, 1, false);

    stream = json_parse_whitespace(stream);
    if (*(stream.s) == '}') {
        stream = json_stream_advance(stream, 1, false);
        x->child = NULL;
        x->type = object_t;
        return stream;
    }

    struct json * y = json_element();
    if (!y) {
        make_json_parser_error(x, stream, strerror(errno));
        return stream;
    }

    stream = json_parse_member(y, stream);
    if (json_is_invalid(y)) {
        copy_json_parser_error(x, y);
        json_free(y);
        return stream;
    }

    struct json * z = y;
    while (*(stream.s) == ',') {
        stream = json_stream_advance(stream, 1, false);
        z->next = json_element();
        if (!z->next) {
            make_json_parser_error(x, stream, strerror(errno));
            json_free(y);
            return stream;
        }

        stream = json_parse_member(z->next, stream);
        if (json_is_invalid(z->next)) {
            copy_json_parser_error(x, z->next);
            json_free(y);
            return stream;
        }

        z->next->prev = z;
        z = z->next;
    }

    if (*(stream.s) != '}') {
        make_json_parser_error(
            x, stream, "Ill-formed expression: Expected '}'");
        json_free(y);
        return stream;
    }
    stream = json_stream_advance(stream, 1, false);
    x->child = y;
    x->type = object_t;
    return stream;
}

struct json_stream json_parse_value(struct json * x, struct json_stream stream)
{
    if (strncmp(stream.s, "null", strlen("null")) == 0) {
        x->type = null_t;
        stream = json_stream_advance(stream, strlen("null"), false);
        return stream;
    }
    else if (strncmp(stream.s, "true", strlen("true")) == 0) {
        x->type = true_t;
        stream = json_stream_advance(stream, strlen("true"), false);
        return stream;
    }
    else if (strncmp(stream.s, "false", strlen("false")) == 0) {
        x->type = false_t;
        stream = json_stream_advance(stream, strlen("false"), false);
        return stream;
    }
    else if (json_is_digit(*(stream.s)) || *(stream.s) == '-')
        return json_parse_number(x, stream);
    else if (*(stream.s) == '"')
        return json_parse_string(x, stream);
    else if (*(stream.s) == '[')
        return json_parse_array(x, stream);
    else if (*(stream.s) == '{')
        return json_parse_object(x, stream);

    make_json_parser_error(
        x, stream,
        "Ill-formed expression: Expected "
        "\"null\", \"true\", \"false\", or "
        "an expression beginning with "
        "'0'-'9', '-', '\"', '[', or '{'");
    return stream;
}

struct json_stream json_parse_element(struct json * x, struct json_stream stream)
{
    stream = json_parse_whitespace(stream);
    stream = json_parse_value(x, stream);
    if (json_is_invalid(x))
        return stream;
    return json_parse_whitespace(stream);
}

struct json * json_parse(const char * s)
{
    struct json * x = json_element();
    if (!x)
        return NULL;
    json_parse_element(x, json_stream(s));
    return x;
}
