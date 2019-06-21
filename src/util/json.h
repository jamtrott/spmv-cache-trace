#ifndef JSON_H
#define JSON_H

/*
 * A parser for the JavaScript Object Notation (JSON) data-interchange format.
 * For more information, see http://www.json.org/.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int json_size_type;

/*
 * Data structure for JSON elements.
 */
struct json
{
    int type;
    double number;
    const char * string;
    struct json * child;
    struct json * next;
    struct json * prev;
};

void json_free(struct json * x);

/*
 * JSON type constructors.
 */
struct json * json_null(void);
struct json * json_true(void);
struct json * json_false(void);
struct json * json_number(double n);
struct json * json_string(const char * s);
struct json * json_array(struct json ** x, json_size_type n);
struct json * json_object(struct json ** x, json_size_type n);
struct json * json_member(const char * key, struct json * value);

/*
 * Type checks for JSON elements.
 */
bool json_is_invalid(const struct json * x);
bool json_is_null(const struct json * x);
bool json_is_true(const struct json * x);
bool json_is_false(const struct json * x);
bool json_is_number(const struct json * x);
bool json_is_string(const struct json * x);
bool json_is_array(const struct json * x);
bool json_is_object(const struct json * x);
bool json_is_member(const struct json * x);

/*
 * Conversion from JSON elements to C data types.
 */
int json_to_int(const struct json * x);
double json_to_double(const struct json * x);
const char * json_to_string(const struct json * x);
const char * json_to_key(const struct json * x);
struct json * json_to_value(const struct json * x);
const char * json_to_error(const struct json * x);

/*
 * JSON arrays.
 */
json_size_type json_array_size(const struct json * x);
struct json * json_array_get(const struct json * x, json_size_type n);
struct json * json_array_begin(const struct json * x);
struct json * json_array_end(void);
struct json * json_array_next(const struct json * x);
struct json * json_array_rbegin(const struct json * x);
struct json * json_array_rend(void);
struct json * json_array_prev(const struct json * x);

/*
 * JSON objects.
 */
json_size_type json_object_size(const struct json * x);
struct json * json_object_get(const struct json * x, const char * key);
struct json * json_object_begin(const struct json * x);
struct json * json_object_end(void);
struct json * json_object_next(const struct json * x);
struct json * json_object_rbegin(const struct json * x);
struct json * json_object_rend(void);
struct json * json_object_prev(const struct json * x);

/*
 * Parse a string to produce a JSON element.
 */
struct json * json_parse(const char * s);

#ifdef __cplusplus
}
#endif

#endif
