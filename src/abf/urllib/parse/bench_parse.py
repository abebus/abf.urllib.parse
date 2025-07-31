import abfparse
import urllib.parse
import timeit

with open("top100.txt", "r") as f:
    URLS = f.readlines()
    S = URLS

print(URLS[0])
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
    t1 = timeit.timeit(bench_abfparse_urlparse, number=10)
    t2 = timeit.timeit(bench_urllib_urlparse, number=10)
    print(f"abfparse.url_parse: {t1:.4f}s, urllib.parse.urlparse: {t2:.4f}s, speedup: {t2/t1:.2f}x")

    print("Benchmarking url_quote...")
    t3 = timeit.timeit(bench_abfparse_quote, number=10)
    t4 = timeit.timeit(bench_urllib_quote, number=10)
    print(f"abfparse.url_quote: {t3:.4f}s, urllib.parse.quote: {t4:.4f}s, speedup: {t4/t3:.2f}x")
