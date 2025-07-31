#include "parse.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

enum {
    URL_SAFE_TABLE_SIZE = 256,
    URL_PERCENT_ENCODED_LEN = 3,
    URL_WHITESPACE_MAX = 0x20, // 0x00 - 0x20 ascii charters are skipped
    NIBBLE_BITS = 4,
    NIBBLE_MASK = 0xF
};

// --- Safe character lookup table ---
static const bool p_URL_SAFE_ALWAYS[URL_SAFE_TABLE_SIZE] = {
    ['A' ... 'Z'] = true, ['a' ... 'z'] = true, ['0' ... '9'] = true,
    ['-'] = true,         ['.'] = true,         ['_'] = true,
    ['~'] = true};

static inline const char *const *p_URL_SCHEMES_WITH_PARAMS(size_t *count) {
    static const char *schemes[] = {
        "",     "ftp",   "hdl",   "prospero", "http", "imap", "https", "shttp",
        "rtsp", "rtsps", "rtspu", "sip",      "sips", "mms",  "sftp",  "tel"};
    if (count) {
        *count = sizeof(schemes) / sizeof(*schemes);
    }
    return schemes;
}

// Helper: percent-encode a byte
static inline void p_percent_encode(char c, char *input, size_t *out_idx) {
    static const char hex[] = "0123456789ABCDEF";
    input[--(*out_idx)] = hex[c & 0xF];
    input[--(*out_idx)] = hex[(c >> 4) & 0xF];
    input[--(*out_idx)] = '%';
}

// Helper: set url_component_t
static inline void p_set_component(url_component_t *comp, const char *start,
                                   size_t len) {
    comp->start = start;
    comp->length = len;
}

// Helper: left-strip C0/control/space
static inline void p_lstrip_ws(const char **s, size_t *len) {
    size_t i = 0, n = *len;
    const char *str = *s;
    while (i < n && (unsigned char)str[i] <= URL_WHITESPACE_MAX) {
        i++;
    }
    *s = str + i;
    *len = n - i;
}

// Helper: right-strip C0/control/space
static inline void p_rstrip_ws(const char **url, size_t *url_len) {
    size_t n = *url_len;
    const char *s = *url;

    // Skip trailing
    while (n > 0 && (unsigned char)s[n - 1] <= URL_WHITESPACE_MAX) {
        n--;
    }

    *url = s;
    *url_len = n;
}

// Helper: remove unsafe bytes from a buffer, write to user buffer
static inline void p_remove_unsafe_bytes(char **url, size_t *url_len) {
    static const char unsafe[] = {'\t', '\r', '\n'};
    char *buf = *url;
    size_t len = *url_len;
    size_t out = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        int is_unsafe = 0;
        for (size_t j = 0; j < sizeof(unsafe); ++j) {
            if (c == unsafe[j]) {
                is_unsafe = 1;
                break;
            }
        }
        if (!is_unsafe) {
            if (out != i) {
                memmove(&buf[out], &buf[i], 1);
            }
            out++;
        }
    }
    *url_len = out;
}

// Helper: validate scheme
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

// Helper: extract netloc
static inline size_t p_extract_netloc(const char *url, size_t url_len) {
    size_t netloc_len = 0;
    while (netloc_len < url_len && url[netloc_len] != '/' &&
           url[netloc_len] != '?' && url[netloc_len] != '#') {
        netloc_len++;
    }
    return netloc_len;
}

// Fast url_split implementation (no unicode, no CPython API)
url_parse_error_t url_split(char *url, size_t url_len, const char *scheme,
                            bool allow_fragments, url_split_result_t *result) {
    // Initialize all components to empty
    p_set_component(&result->scheme, NULL, 0);
    p_set_component(&result->netloc, NULL, 0);
    p_set_component(&result->path, NULL, 0);
    p_set_component(&result->query, NULL, 0);
    p_set_component(&result->fragment, NULL, 0);

    // 1. Strip leading spaces/control chars (WHATWG)
    p_lstrip_ws((const char **)&url, &url_len);
    // Remove unsafe chars in-place
    p_remove_unsafe_bytes(&url, &url_len);

    // 2. Scheme detection
    const char *colon = memchr(url, ':', url_len);
    if (colon) {
        // Prepare scheme for validation
        const char *scheme_start = url;
        size_t scheme_len = (size_t)(colon - url);
        p_lstrip_ws(&scheme_start, &scheme_len); // Strip both sides for scheme
        p_rstrip_ws(&scheme_start, &scheme_len); // Strip both sides for scheme
        if (p_is_valid_scheme(scheme_start, scheme_start + scheme_len)) {
            p_set_component(&result->scheme, scheme_start, scheme_len);
            url_len -= (colon - url + 1);
            url = (char *)colon + 1;
        } else if (scheme && scheme_len) {
            p_set_component(&result->scheme, scheme, scheme_len);
        }
    }

    // 3. Netloc
    if (url_len >= 2 && url[0] == '/' && url[1] == '/') {
        url += 2;
        url_len -= 2;
        size_t netloc_len = p_extract_netloc(url, url_len);
        p_set_component(&result->netloc, url, netloc_len);
        url += netloc_len;
        url_len -= netloc_len;
    }

    // 4. Fragment
    if (allow_fragments) {
        const char *frag = memchr(url, '#', url_len);
        if (frag) {
            p_set_component(&result->fragment, frag + 1,
                            url + url_len - (frag + 1));
            url_len = frag - url;
        }
    }

    // 5. Query
    const char *qmark = memchr(url, '?', url_len);
    if (qmark) {
        p_set_component(&result->query, qmark + 1, url + url_len - (qmark + 1));
        url_len = qmark - url;
    }

    // 6. Path
    p_set_component(&result->path, url, url_len);
    return URL_PARSE_OK;
}

// url_parse: also split params (Python legacy)
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
    int uses_params = 0;
    const char *result_scheme = result->scheme.start;
    size_t result_scheme_len = result->scheme.length;
    size_t scheme_count = 0;
    const char *const *schemes = p_URL_SCHEMES_WITH_PARAMS(&scheme_count);
    if (result_scheme && result_scheme_len) {
        for (size_t i = 0; i < scheme_count; ++i) {
            size_t slen = strlen(schemes[i]);
            if (result_scheme_len == slen &&
                strncmp(result_scheme, schemes[i], slen) == 0) {
                uses_params = 1;
                break;
            }
        }
    }
    if (uses_params) {
        const char *semicolon =
            memchr(result->path.start, ';', result->path.length);
        if (semicolon) {
            size_t plen = (size_t)(semicolon - result->path.start);
            url_component_t params_tmp;
            p_set_component(&params_tmp, semicolon + 1,
                            result->path.length - plen - 1);
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
url_parse_error_t url_quote(char *buf, size_t orig_str_len, const char *safe,
                            size_t safe_len) {
    // In-place percent-encode: since encoding expands, must process backwards
    // 1. Calculate needed space
    size_t needed = 0;
    for (size_t i = 0; i < orig_str_len; ++i) {
        char c = buf[i];
        bool is_safe = p_URL_SAFE_ALWAYS[(unsigned char)c] ||
                       (safe && safe_len && memchr(safe, c, safe_len));
        needed += is_safe ? 1 : 3;
    }
    // 2. Check if buffer is large enough (should always be true for VLA caller)
    // 3. Process backwards for in-place percent-encoding
    size_t inp_idx = orig_str_len;
    size_t out_idx = needed;
    buf[out_idx] = '\0'; // Null-terminate
    while (inp_idx > 0) {
        char c = buf[--inp_idx];
        bool is_safe = p_URL_SAFE_ALWAYS[(unsigned char)c] ||
                       (safe && safe_len && memchr(safe, c, safe_len));
        if (is_safe) {
            buf[--out_idx] = c;
        } else {
            p_percent_encode(c, buf, &out_idx);
        }
    }
    return URL_PARSE_OK;
}
