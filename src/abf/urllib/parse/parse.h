#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include <string.h>

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

/* Error codes */
typedef enum {
    URL_PARSE_OK = 0,
    URL_PARSE_ERROR_INVALID_IPV6 = 1,
    URL_PARSE_ERROR_INVALID_NETLOC = 2,
    URL_PARSE_ERROR_OUT_OF_MEMORY = 3,
    URL_PARSE_ERROR_INVALID_INPUT = 4,
    URL_PARSE_ERROR_UNKNOWN = 64
} url_parse_error_t;

/* Core parsing functions */
url_parse_error_t url_parse(char *url, size_t url_len, const char *scheme,
                            bool allow_fragments, url_parse_result_t *result);

url_parse_error_t url_split(char *url, size_t url_len, const char *scheme,
                            bool allow_fragments, url_split_result_t *result);

/* Quote function. */
url_parse_error_t url_quote(char *input, size_t input_len, const char *safe,
                            size_t safe_len);

#endif
