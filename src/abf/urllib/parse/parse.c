
#include "parse.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper: check if a char is in a set
static int char_in(const char *set, size_t set_len, char c) {
    for (size_t i = 0; i < set_len; ++i) {
        if (set[i] == c) return 1;
    }
    return 0;
}


// Helper: set url_component_t
static void set_component(url_component_t *comp, const char *start, size_t len) {
    comp->start = start;
    comp->length = len;
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
    size_t i = 0;
    while (i < url_len && (unsigned char)url[i] <= 0x20) i++;
    url += i;
    url_len -= i;

    // 2. Scheme detection
    const char *colon = memchr(url, ':', url_len);
    if (colon && colon > url && isalpha(url[0])) {
        // Check scheme chars
        int valid = 1;
        for (const char *p = url; p < colon; ++p) {
            if (!(isalnum(*p) || *p == '+' || *p == '-' || *p == '.')) {
                valid = 0; break;
            }
        }
        if (valid) {
            set_component(&result->scheme, url, colon - url);
            url_len -= (colon - url + 1);
            url = colon + 1;
        }
    } else if (scheme && scheme_len) {
        set_component(&result->scheme, scheme, scheme_len);
    }

    // 3. Netloc
    if (url_len >= 2 && url[0] == '/' && url[1] == '/') {
        url += 2; url_len -= 2;
        size_t netloc_len = 0;
        while (netloc_len < url_len && url[netloc_len] != '/' && url[netloc_len] != '?' && url[netloc_len] != '#') netloc_len++;
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
    const char *sc = result->scheme.start;
    size_t sclen = result->scheme.length;
    int uses_params = 0;
    if (sc && sclen) {
        // Only check for http, https, ftp, etc. (simplified)
        if ((sclen == 4 && strncmp(sc, "http", 4) == 0) ||
            (sclen == 5 && strncmp(sc, "https", 5) == 0) ||
            (sclen == 3 && strncmp(sc, "ftp", 3) == 0)) {
            uses_params = 1;
        }
    }
    if (uses_params) {
        const char *semi = memchr(result->path.start, ';', result->path.length);
        if (semi) {
            size_t plen = semi - result->path.start;
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

// url_quote: percent-encode all except "safe" chars
url_parse_error_t url_quote(const char *input, size_t input_len,
                           const char *safe, size_t safe_len,
                           quote_result_t *result) {
    // RFC 3986 unreserved: ALPHA / DIGIT / "-._~"
    const char *always_safe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    size_t always_safe_len = 66;
    size_t cap = input_len * 3 + 1;
    char *out = (char *)malloc(cap);
    if (!out) return URL_PARSE_ERROR_OUT_OF_MEMORY;
    size_t j = 0;
    for (size_t i = 0; i < input_len; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (char_in(always_safe, always_safe_len, c) || char_in(safe, safe_len, c)) {
            out[j++] = c;
        } else {
            if (j + 3 >= cap) {
                cap *= 2;
                char *tmp = (char *)realloc(out, cap);
                if (!tmp) { free(out); return URL_PARSE_ERROR_OUT_OF_MEMORY; }
                out = tmp;
            }
            out[j++] = '%';
            static const char hex[] = "0123456789ABCDEF";
            out[j++] = hex[c >> 4];
            out[j++] = hex[c & 0xF];
        }
    }
    out[j] = '\0';
    result->data = out;
    result->length = j;
    result->capacity = cap;
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
