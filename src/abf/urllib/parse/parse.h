#ifndef ABF_URLLIB_PARSE_H
#define ABF_URLLIB_PARSE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* URL component structure */
typedef struct {
    const char *start;
    size_t length;
} url_component_t;

/* URL parse result structure */
typedef struct {
    url_component_t scheme;
    url_component_t netloc;
    url_component_t path;
    url_component_t params;
    url_component_t query;
    url_component_t fragment;
    bool has_params;
} url_parse_result_t;

/* URL split result structure (without params) */
typedef struct {
    url_component_t scheme;
    url_component_t netloc;
    url_component_t path;
    url_component_t query;
    url_component_t fragment;
} url_split_result_t;

/* Quote result structure */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} quote_result_t;

/* Error codes */
typedef enum {
    URL_PARSE_OK = 0,
    URL_PARSE_ERROR_INVALID_IPV6 = -1,
    URL_PARSE_ERROR_INVALID_NETLOC = -2,
    URL_PARSE_ERROR_OUT_OF_MEMORY = -3,
    URL_PARSE_ERROR_INVALID_INPUT = -4
} url_parse_error_t;

/* Core parsing functions */
url_parse_error_t url_parse(const char *url, size_t url_len, 
                           const char *scheme, size_t scheme_len,
                           bool allow_fragments, url_parse_result_t *result);

url_parse_error_t url_split(const char *url, size_t url_len, 
                           const char *scheme, size_t scheme_len,
                           bool allow_fragments, url_split_result_t *result);

/* Quote/unquote functions */
url_parse_error_t url_quote(const char *input, size_t input_len,
                           const char *safe, size_t safe_len,
                           quote_result_t *result);

url_parse_error_t url_unquote(const char *input, size_t input_len,
                             quote_result_t *result);

url_parse_error_t url_quote_plus(const char *input, size_t input_len,
                                const char *safe, size_t safe_len,
                                quote_result_t *result);

url_parse_error_t url_unquote_plus(const char *input, size_t input_len,
                                  quote_result_t *result);

/* Utility functions */
void url_component_copy(const url_component_t *src, char *dest, size_t dest_size);
bool url_component_equals(const url_component_t *a, const char *b, size_t b_len);
void quote_result_free(quote_result_t *result);

/* Scheme classification functions */
bool scheme_uses_relative(const char *scheme, size_t len);
bool scheme_uses_netloc(const char *scheme, size_t len);
bool scheme_uses_params(const char *scheme, size_t len);
bool scheme_uses_query(const char *scheme, size_t len);
bool scheme_uses_fragment(const char *scheme, size_t len);

#endif /* ABF_URLLIB_PARSE_H */
