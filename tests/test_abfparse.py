from pathlib import Path
import abf.urllib.parse
import urllib.parse
import pytest
import os

def discover_txt_files(root=Path(__file__).parent):
    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if fname.endswith('.txt'):
                yield os.path.join(dirpath, fname)


@pytest.fixture(params=discover_txt_files())
def urls_file(request):
    filename = request.param
    with open(filename) as f:
        yield f

def test_abfparse_matches_stdlib(urls_file):
    for url in urls_file:
        abfparseres = abf.urllib.parse.urlparse(url)
        pyres = urllib.parse.urlparse(url)
        assert isinstance(abfparseres, type(pyres)), f"Type: {type(abfparseres)} vs {type(pyres)}"
        assert abfparseres.scheme == pyres.scheme, f"Scheme: {abfparseres.scheme!r} != {pyres.scheme!r}"
        assert abfparseres.netloc == pyres.netloc, f"Netloc: {abfparseres.netloc!r} != {pyres.netloc!r}"
        assert abfparseres.path == pyres.path, f"Path: {abfparseres.path!r} != {pyres.path!r}"
        assert abfparseres.params == pyres.params, f"Params: {abfparseres.params!r} != {pyres.params!r}"
        assert abfparseres.query == pyres.query, f"Query: {abfparseres.query!r} != {pyres.query!r}"
        assert abfparseres.fragment == pyres.fragment, f"Fragment: {abfparseres.fragment!r} != {pyres.fragment!r}"
        assert abfparseres == pyres, f"ParseResult: {abfparseres!r}"

def test_abfparse_urlsplit_matches_stdlib(urls_file):
    for url in urls_file:
        abfparseres = abf.urllib.parse.urlsplit(url)
        pyres = urllib.parse.urlsplit(url)
        assert isinstance(abfparseres, type(pyres)), f"Type: {type(abfparseres)} vs {type(pyres)}"
        assert abfparseres.scheme == pyres.scheme, f"Scheme: {abfparseres.scheme!r} != {pyres.scheme!r}"
        assert abfparseres.netloc == pyres.netloc, f"Netloc: {abfparseres.netloc!r} != {pyres.netloc!r}"
        assert abfparseres.path == pyres.path, f"Path: {abfparseres.path!r} != {pyres.path!r}"
        assert abfparseres.query == pyres.query, f"Query: {abfparseres.query!r} != {pyres.query!r}"
        assert abfparseres.fragment == pyres.fragment, f"Fragment: {abfparseres.fragment!r} != {pyres.fragment!r}"
        assert abfparseres == pyres, f"SplitResult: {abfparseres!r}"

def test_abfparse_quote_matches_stdlib():
    # Test a variety of cases
    cases = [
        ("abc def", "/"),
        ("/foo/bar?baz=qux", "/"),
        ("a+b@c.com", "@"),
        ("100% legit", "%"),
        ("", "/"),
        ("/unsafe\nnewline", "/"),
    ]
    for s, safe in cases:
        abfres = abf.urllib.parse.quote(s, safe)
        pyres = urllib.parse.quote(s, safe)
        assert abfres == pyres, f"quote({s!r}, {safe!r}): {abfres!r} != {pyres!r}"

def test_bench_urlparse(benchmark, urls_file):
    url = urls_file.readline()
    benchmark(lambda: urllib.parse.urlparse(url))

def test_bench_abfparseurlparse(benchmark, urls_file):
    url = urls_file.readline()
    benchmark(lambda: abf.urllib.parse.urlparse(url))