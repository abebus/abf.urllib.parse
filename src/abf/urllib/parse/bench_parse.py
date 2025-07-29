import abfparse
import urllib.parse
import timeit

URLS = [
    "http://user:pass@host:80/path;params?query=1#frag",
    "https://example.com/foo/bar?baz=qux#frag",
    "ftp://ftp.example.com/resource;type=i",
    "http://localhost:8080/abc/def?x=1&y=2#zzz",
    "https://user@host.com:443/path/to/resource;param?query=val#frag",
    "http://[::1]/ipv6/path;params?query=1#frag",
    "http://user:pass@host:80/path;params?query=1#frag" * 2,  # long url
]

S = [
    "abc def/!@#%&*()_+",
    "a" * 100,
    "a b c d e f g h i j k l m n o p q r s t u v w x y z",
    "!" * 50,
    "/foo/bar/baz?x=1&y=2#frag",
]

def bench_abfparse_urlparse():
    for url in URLS:
        abfparse.url_parse(url)

def bench_urllib_urlparse():
    for url in URLS:
        urllib.parse.urlparse(url)

def bench_abfparse_quote():
    for s in S:
        abfparse.url_quote(s)

def bench_urllib_quote():
    for s in S:
        urllib.parse.quote(s)

if __name__ == "__main__":
    print("Benchmarking url_parse...")
    t1 = timeit.timeit(bench_abfparse_urlparse, number=100000)
    t2 = timeit.timeit(bench_urllib_urlparse, number=100000)
    print(f"abfparse.url_parse: {t1:.4f}s, urllib.parse.urlparse: {t2:.4f}s, speedup: {t2/t1:.2f}x")

    print("Benchmarking url_quote...")
    t3 = timeit.timeit(bench_abfparse_quote, number=100000)
    t4 = timeit.timeit(bench_urllib_quote, number=100000)
    print(f"abfparse.url_quote: {t3:.4f}s, urllib.parse.quote: {t4:.4f}s, speedup: {t4/t3:.2f}x")
