import functools
from urllib.parse import _coerce_args, _WHATWG_C0_CONTROL_OR_SPACE, _splitnetloc, _check_bracketed_netloc, _UNSAFE_URL_BYTES_TO_REMOVE, uses_params, ParseResult

UNSAFE_BYTES = b''.join(_UNSAFE_URL_BYTES_TO_REMOVE)
TRANSLATE_TABLE_STR_BYTES = bytes.maketrans(b'', b'', UNSAFE_BYTES)
UNSAFE_CHARS = ''.join(_UNSAFE_URL_BYTES_TO_REMOVE)
TRANSLATE_TABLE_STR = str.maketrans('', '', UNSAFE_CHARS)

@functools.lru_cache(typed=True)
def urlsplit(url, scheme='', allow_fragments=True):
    """Parse a URL into 5 components:
    <scheme>://<netloc>/<path>?<query>#<fragment>

    The result is a named 5-tuple with fields corresponding to the
    above. It is either a SplitResult or SplitResultBytes object,
    depending on the type of the url parameter.

    The username, password, hostname, and port sub-components of netloc
    can also be accessed as attributes of the returned object.

    The scheme argument provides the default value of the scheme
    component when no scheme is found in url.

    If allow_fragments is False, no attempt is made to separate the
    fragment component from the previous component, which can be either
    path or query.

    Note that % escapes are not expanded.
    """

    url, scheme, _coerce_result = _coerce_args(url, scheme)
    # Only lstrip url as some applications rely on preserving trailing space.
    url = url.lstrip(_WHATWG_C0_CONTROL_OR_SPACE)
    if scheme:
        scheme = scheme.strip(_WHATWG_C0_CONTROL_OR_SPACE)

    url = url.translate(TRANSLATE_TABLE_STR) if isinstance(url, str) else url.translate(TRANSLATE_TABLE_STR_BYTES)
    if scheme:
        scheme = scheme.translate(TRANSLATE_TABLE_STR) if isinstance(scheme, str) else scheme.translate(TRANSLATE_TABLE_STR_BYTES)

    allow_fragments = bool(allow_fragments)
    netloc = query = fragment = ''
    i = url.find(':')
    if i > 0 and url[0].isascii() and url[0].isalpha():
        for c in url[:i]:
            if c not in scheme_chars:
                break
        else:
            scheme, url = url[:i].lower(), url[i+1:]

    # Avoid unnecessary string slices
    if len(url) >= 2 and url[0] == '/' and url[1] == '/':
        netloc, url = _splitnetloc(url, 2)
        # IPv6 netloc validation, more readable and efficient
        has_open = '[' in netloc
        has_close = ']' in netloc
        if has_open != has_close:
            raise ValueError("Invalid IPv6 URL")
        if has_open:
            _check_bracketed_netloc(netloc)

    # Single scan for fragment and query
    path = url
    if allow_fragments:
        hash_idx = path.find('#')
        if hash_idx != -1:
            fragment = path[hash_idx+1:]
            path = path[:hash_idx]
    q_idx = path.find('?')
    if q_idx != -1:
        query = path[q_idx+1:]
        path = path[:q_idx]

    _checknetloc(netloc)
    v = SplitResult(scheme, netloc, path, query, fragment)
    return _coerce_result(v)

def urlparse(url, scheme='', allow_fragments=True):
    """Parse a URL into 6 components:
    <scheme>://<netloc>/<path>;<params>?<query>#<fragment>

    The result is a named 6-tuple with fields corresponding to the
    above. It is either a ParseResult or ParseResultBytes object,
    depending on the type of the url parameter.

    The username, password, hostname, and port sub-components of netloc
    can also be accessed as attributes of the returned object.

    The scheme argument provides the default value of the scheme
    component when no scheme is found in url.

    If allow_fragments is False, no attempt is made to separate the
    fragment component from the previous component, which can be either
    path or query.

    Note that % escapes are not expanded.
    """
    url, scheme, _coerce_result = _coerce_args(url, scheme)
    splitresult = urlsplit(url, scheme, allow_fragments)
    scheme, netloc, url, query, fragment = splitresult
    if scheme in uses_params and ';' in url:
        url, params = _splitparams(url)
    else:
        params = ''
    result = ParseResult(scheme, netloc, url, params, query, fragment)
    return _coerce_result(result)