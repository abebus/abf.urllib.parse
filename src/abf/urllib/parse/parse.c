

#include "parse.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- Constants ---
static inline const char *URL_SAFE_ALWAYS() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
}
static inline size_t URL_SAFE_ALWAYS_LEN() { return 66; }

static inline const char *URL_SCHEMES_WITH_PARAMS(size_t *count) {
    static const char *schemes[] = {"http", "https", "ftp"};
    if (count) *count = sizeof(schemes)/sizeof(*schemes);
    return (const char *)schemes;
}


// Helper: check if a char is in a set
static inline int char_in(const char *set, size_t set_len, char c) {
    for (size_t i = 0; i < set_len; ++i) {
        if (set[i] == c) return 1;
    }
    return 0;
}

// Helper: set url_component_t
static inline void set_component(url_component_t *comp, const char *start, size_t len) {
    comp->start = start;
    comp->length = len;
}

// Helper: skip leading C0/control/space
static inline void skip_leading_ws(const char **url, size_t *url_len) {
    size_t i = 0;
    while (i < *url_len && (unsigned char)(*url)[i] <= 0x20) i++;
    *url += i;
    *url_len -= i;
}

// Helper: validate scheme
static inline int is_valid_scheme(const char *url, const char *colon) {
    if (!url || !colon || colon <= url) return 0;
    if (!isalpha(url[0])) return 0;
    for (const char *p = url; p < colon; ++p) {
        if (!(isalnum(*p) || *p == '+' || *p == '-' || *p == '.'))
            return 0;
    }
    return 1;
}

// Helper: extract netloc
static inline size_t extract_netloc(const char *url, size_t url_len) {
    size_t netloc_len = 0;
    while (netloc_len < url_len && url[netloc_len] != '/' && url[netloc_len] != '?' && url[netloc_len] != '#')
        netloc_len++;
    return netloc_len;
}

// Fast url_split implementation (no unicode, no CPython API)
url_parse_error_t url_split(const char *url, size_t url_len,
                           const char *scheme, size_t scheme_len,
                           bool allow_fragments, url_split_result_t *result) {
    // Initialize all components to empty
    set_component(&result->scheme, NULL, 0);
    set_component(&result->netloc, NULL, 0);
    set_component(&result->path, NULL, 0);
    set_component(&result->query, NULL, 0);
    set_component(&result->fragment, NULL, 0);

    // 1. Strip leading spaces/control chars (WHATWG)
    skip_leading_ws(&url, &url_len);

    // 2. Scheme detection
    const char *colon = memchr(url, ':', url_len);
    if (is_valid_scheme(url, colon)) {
        set_component(&result->scheme, url, colon - url);
        url_len -= (colon - url + 1);
        url = colon + 1;
    } else if (scheme && scheme_len) {
        set_component(&result->scheme, scheme, scheme_len);
    }

    // 3. Netloc
    if (url_len >= 2 && url[0] == '/' && url[1] == '/') {
        url += 2; url_len -= 2;
        size_t netloc_len = extract_netloc(url, url_len);
        set_component(&result->netloc, url, netloc_len);
        url += netloc_len; url_len -= netloc_len;
    }

    // 4. Fragment
    if (allow_fragments) {
        const char *hash = memchr(url, '#', url_len);
        if (hash) {
            set_component(&result->fragment, hash + 1, url + url_len - (hash + 1));
            url_len = hash - url;
        }
    }

    // 5. Query
    const char *qmark = memchr(url, '?', url_len);
    if (qmark) {
        set_component(&result->query, qmark + 1, url + url_len - (qmark + 1));
        url_len = qmark - url;
    }

    // 6. Path
    set_component(&result->path, url, url_len);
    return URL_PARSE_OK;
}

// url_parse: also split params (Python legacy)
url_parse_error_t url_parse(const char *url, size_t url_len,
                           const char *scheme, size_t scheme_len,
                           bool allow_fragments, url_parse_result_t *result) {
    url_split_result_t split;
    url_parse_error_t err = url_split(url, url_len, scheme, scheme_len, allow_fragments, &split);
    if (err != URL_PARSE_OK) return err;
    // Copy split results
    result->scheme = split.scheme;
    result->netloc = split.netloc;
    result->path = split.path;
    result->query = split.query;
    result->fragment = split.fragment;
    result->has_params = 0;

    // Params: only if scheme uses params and ';' in path
    int uses_params = 0;
    const char *sc = result->scheme.start;
    size_t sclen = result->scheme.length;
    size_t scheme_count = 0;
    const char *const *schemes = (const char *const *)URL_SCHEMES_WITH_PARAMS(&scheme_count);
    if (sc && sclen) {
        for (size_t i = 0; i < scheme_count; ++i) {
            size_t slen = strlen(schemes[i]);
            if (sclen == slen && strncmp(sc, schemes[i], slen) == 0) {
                uses_params = 1;
                break;
            }
        }
    }
    if (uses_params) {
        const char *semi = memchr(result->path.start, ';', result->path.length);
        if (semi) {
            size_t plen = (size_t)(semi - result->path.start);
            set_component(&result->params, semi + 1, result->path.length - plen - 1);
            result->path.length = plen;
            result->has_params = 1;
        } else {
            set_component(&result->params, NULL, 0);
        }
    } else {
        set_component(&result->params, NULL, 0);
    }
    return URL_PARSE_OK;
}

// Helper: percent-encode a byte
static inline void percent_encode(char c, char *out, size_t *j, size_t *cap, char **buf) {
    static const char hex[] = "0123456789ABCDEF";
    if (*j + 3 >= *cap) {
        *cap *= 2;
        char *tmp = (char *)realloc(*buf, *cap);
        if (!tmp) return;
        *buf = tmp;
        out = *buf + *j;
    }
    out[(*j)++] = '%';
    out[(*j)++] = hex[(c >> 4) & 0xF];
    out[(*j)++] = hex[c & 0xF];
}

// url_quote: percent-encode all except "safe" chars, use stack buffer for small strings
#define URL_QUOTE_STACKBUF 256
url_parse_error_t url_quote(const char *input, size_t input_len,
                           const char *safe, size_t safe_len,
                           quote_result_t *result) {
    const char *always_safe = URL_SAFE_ALWAYS();
    size_t always_safe_len = URL_SAFE_ALWAYS_LEN();
    size_t cap = input_len * 3 + 1;
    char stackbuf[URL_QUOTE_STACKBUF];
    char *out = stackbuf;
    char *buf = out;
    int used_heap = 0;
    if (cap > URL_QUOTE_STACKBUF) {
        out = (char *)malloc(cap);
        if (!out) return URL_PARSE_ERROR_OUT_OF_MEMORY;
        buf = out;
        used_heap = 1;
    }
    size_t j = 0;
    for (size_t i = 0; i < input_len; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (char_in(always_safe, always_safe_len, c) || char_in(safe, safe_len, c)) {
            out[j++] = c;
        } else {
            percent_encode(c, out, &j, &cap, &buf);
            out = buf;
            if (!used_heap && cap > URL_QUOTE_STACKBUF) {
                // Switch to heap if buffer grew
                out = (char *)malloc(cap);
                if (!out) return URL_PARSE_ERROR_OUT_OF_MEMORY;
                memcpy(out, stackbuf, j);
                buf = out;
                used_heap = 1;
            }
        }
    }
    out[j] = '\0';
    if (!used_heap) {
        // Copy to heap for API contract
        char *heapbuf = (char *)malloc(j + 1);
        if (!heapbuf) return URL_PARSE_ERROR_OUT_OF_MEMORY;
        memcpy(heapbuf, out, j + 1);
        result->data = heapbuf;
        result->capacity = j + 1;
    } else {
        result->data = out;
        result->capacity = cap;
    }
    result->length = j;
    return URL_PARSE_OK;
}

void quote_result_free(quote_result_t *result) {
    if (result && result->data) {
        free(result->data);
        result->data = NULL;
        result->length = 0;
        result->capacity = 0;
    }
}
