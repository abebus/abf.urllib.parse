#define _GNU_SOURCE
#include "parse.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

enum {
    ASCII_SIZE = 256,
    URL_PERCENT_ENCODED_LEN = 3,
    URL_WHITESPACE_LAST =
        0x20, // WHATWG: 0x00 - 0x20 ascii characters are stripped
    NIBBLE_BITS = 4,
    NIBBLE_MASK = 0xF
};
// clang-format off
static const bool p_URL_SAFE_ALWAYS[ASCII_SIZE] = {
    ['A' ... 'Z'] = true,
    ['a' ... 'z'] = true,
    ['0' ... '9'] = true,
    ['-'] = true,
    ['.'] = true,
    ['_'] = true,
    ['~'] = true
};
// clang-format on
typedef struct {
    const char *name;
    size_t len;
} scheme_with_params_t;

static inline const scheme_with_params_t *
p_URL_SCHEMES_WITH_PARAMS(size_t *count) {
    static const scheme_with_params_t schemes[] = {
        {"", 0},     {"ftp", 3},   {"hdl", 3},   {"prospero", 8},
        {"http", 4}, {"imap", 4},  {"https", 5}, {"shttp", 5},
        {"rtsp", 4}, {"rtsps", 5}, {"rtspu", 5}, {"sip", 3},
        {"sips", 4}, {"mms", 3},   {"sftp", 4},  {"tel", 3}};
    if (count) {
        *count = sizeof(schemes) / sizeof(*schemes);
    }
    return schemes;
}

static inline void p_percent_encode(char c, char *input, size_t *out_idx) {
    static const char hex[] = "0123456789ABCDEF";
    input[--(*out_idx)] = hex[c & NIBBLE_MASK];
    input[--(*out_idx)] = hex[(c >> NIBBLE_BITS) & NIBBLE_MASK];
    input[--(*out_idx)] = '%';
}

static inline void p_set_component(url_component_t *comp, const char *start,
                                   size_t len) {
    comp->start = start;
    comp->length = len;
}

static inline void p_lstrip_ws(const char **url, size_t *url_len) {
    size_t i = 0, n = *url_len;
    const char *tmp_str = *url;
    while (i < n && (unsigned char)tmp_str[i] <= URL_WHITESPACE_LAST) {
        i++;
    }
    *url = tmp_str + i;
    *url_len = n - i;
}

static inline void p_rstrip_ws(const char **url, size_t *url_len) {
    size_t n = *url_len;
    const char *tmp_str = *url;

    while (n > 0 && (unsigned char)tmp_str[n - 1] <= URL_WHITESPACE_LAST) {
        n--;
    }

    *url = tmp_str;
    *url_len = n;
}

static const bool p_UNSAFE_CHARS[ASCII_SIZE] = {
    ['\t'] = true, ['\r'] = true, ['\n'] = true};

// Helper: remove unsafe bytes from a buffer inline
static inline void p_remove_unsafe_bytes(char **url, size_t *url_len) {
    char *buf = *url;
    size_t len = *url_len;
    size_t out = 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned const char c = buf[i];
        bool is_unsafe = p_UNSAFE_CHARS[c];
        if (!is_unsafe) {
            if (out != i) {
                buf[out] = buf[i];
            }
            out++;
        }
    }
    *url_len = out;
}

static inline bool p_is_valid_scheme(const char *url, const char *colon) {
    if (!url || !colon || colon <= url) {
        return false;
    }
    if (!isalpha(url[0])) {
        return false;
    }
    for (const char *p = url; p < colon; ++p) {
        if (!(isalnum(*p) || *p == '+' || *p == '-' || *p == '.')) {
            return false;
        }
    }
    return true;
}

static inline size_t p_extract_netloc(const char *url, size_t url_len) {
    size_t netloc_len = 0;
    while (netloc_len < url_len && url[netloc_len] != '/' &&
           url[netloc_len] != '?' && url[netloc_len] != '#') {
        netloc_len++;
    }
    return netloc_len;
}

// Faster url_split implementation (no unicode, no CPython API)
url_parse_error_t url_split(char *url, size_t url_len, const char *scheme,
                            bool allow_fragments, url_split_result_t *result) {
    // Init
    p_set_component(&result->scheme, NULL, 0);
    p_set_component(&result->netloc, NULL, 0);
    p_set_component(&result->path, NULL, 0);
    p_set_component(&result->query, NULL, 0);
    p_set_component(&result->fragment, NULL, 0);

    // Strip only leading spaces/control chars (WHATWG also strip trailing) as
    // urllib.parse does
    p_lstrip_ws((const char **)&url, &url_len);
    p_remove_unsafe_bytes(&url, &url_len);

    // Scheme detection
    const char *colon = memchr(url, ':', url_len);
    if (colon) {
        // Prepare scheme for validation
        const char *scheme_start = url;
        size_t scheme_len = (size_t)(colon - url);
        p_lstrip_ws(
            &scheme_start,
            &scheme_len); // Strip both sides for scheme as urllib.parse does
        p_rstrip_ws(&scheme_start, &scheme_len);
        if (p_is_valid_scheme(scheme_start, scheme_start + scheme_len)) {
            p_set_component(&result->scheme, scheme_start, scheme_len);
            url_len -= (colon - url + 1);
            url = (char *)colon + 1;
        } else if (scheme && scheme_len) {
            p_set_component(&result->scheme, scheme, scheme_len);
        }
    }

    // Netloc
    if (url_len >= 2 && url[0] == '/' && url[1] == '/') {
        url += 2;
        url_len -= 2;
        size_t netloc_len = p_extract_netloc(url, url_len);
        p_set_component(&result->netloc, url, netloc_len);
        url += netloc_len;
        url_len -= netloc_len;
    }

    // Fragment
    if (allow_fragments) {
        const char *frag = memchr(url, '#', url_len);
        if (frag) {
            p_set_component(&result->fragment, frag + 1,
                            url + url_len - (frag + 1));
            url_len = frag - url;
        }
    }

    // Query
    const char *qmark = memchr(url, '?', url_len);
    if (qmark) {
        p_set_component(&result->query, qmark + 1, url + url_len - (qmark + 1));
        url_len = qmark - url;
    }

    // Path
    p_set_component(&result->path, url, url_len);
    return URL_PARSE_OK;
}

// url_parse: also split params as urllib.parse does
url_parse_error_t url_parse(char *url, size_t url_len, const char *scheme,
                            bool allow_fragments, url_parse_result_t *result) {
    url_split_result_t split;
    url_parse_error_t err =
        url_split(url, url_len, scheme, allow_fragments, &split);
    if (err != URL_PARSE_OK) {
        return err;
    }
    // Copy split results
    result->scheme = split.scheme;
    result->netloc = split.netloc;
    result->path = split.path;
    result->query = split.query;
    result->fragment = split.fragment;
    result->has_params = false;

    // Params: only if scheme uses params and ';' in path
    bool uses_params = false;
    const char *result_scheme = result->scheme.start;
    size_t result_scheme_len = result->scheme.length;
    size_t scheme_count = 0;
    const scheme_with_params_t *schemes =
        p_URL_SCHEMES_WITH_PARAMS(&scheme_count);
    if (result_scheme && result_scheme_len) {
        for (size_t i = 0; i < scheme_count; ++i) {
            if (result_scheme_len == schemes[i].len &&
                strncmp(result_scheme, schemes[i].name, schemes[i].len) == 0) {
                uses_params = true;
                break;
            }
        }
    }
    if (uses_params) {
        // Only split params at the first ';' in the last segment of the path
        // (after last '/') as urllib.parse does
        const char *path_start = result->path.start;
        size_t path_len = result->path.length;

        const char *last_slash = NULL;
        last_slash = memrchr(path_start, '/', path_len); // memrchr (GNU)

        const char *segment_start = last_slash ? last_slash + 1 : path_start;
        size_t segment_len = path_len - (segment_start - path_start);
        const char *semicolon = memchr(segment_start, ';', segment_len);
        if (semicolon) {
            size_t plen = (size_t)(semicolon - path_start);
            url_component_t params_tmp;
            p_set_component(&params_tmp, semicolon + 1, path_len - plen - 1);
            result->params = params_tmp;
            result->path.length = plen;
            result->has_params = true;
        } else {
            result->params.start = NULL;
            result->params.length = 0;
        }
    } else {
        result->params.start = NULL;
        result->params.length = 0;
    }
    return URL_PARSE_OK;
}

// url_quote: percent-encode all except "safe" chars inplace
// Note: buf must be with size orig_len * 3 + 1 if we will percent encode whole
// string
url_parse_error_t url_quote(char *buf, size_t orig_str_len, const char *safe,
                            size_t safe_len) {
    // Allocate a temporary array to store "is safe" flags and
    // find new buf cap and calculate which characters are safe
    bool is_safe_arr[orig_str_len];
    size_t needed = 0;
    for (size_t i = 0; i < orig_str_len; ++i) {
        char c = buf[i];
        bool is_safe = p_URL_SAFE_ALWAYS[(unsigned char)c] ||
                       (safe && safe_len && memchr(safe, c, safe_len));
        is_safe_arr[i] = is_safe;
        needed += is_safe ? 1 : URL_PERCENT_ENCODED_LEN;
    }

    size_t inp_idx = orig_str_len;
    size_t out_idx = needed;
    buf[out_idx] = '\0'; // Null-terminate

    while (inp_idx > 0) {
        char c = buf[--inp_idx];
        if (is_safe_arr[inp_idx]) {
            buf[--out_idx] = c;
        } else {
            p_percent_encode(c, buf, &out_idx);
        }
    }

    return URL_PARSE_OK;
}