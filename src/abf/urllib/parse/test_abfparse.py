import abfparse
import urllib.parse
import pytest

test_files = [
    "top100.txt", "userbait.txt", "wikipedia_100k.txt",
    "linux_files.txt", "node_files.txt", "kasztp.txt", "isaacs_files.txt"
]

@pytest.fixture(params=test_files)
def file(request):
    filename = request.param
    with open(filename) as f:
        yield f

def test_abfparse_matches_stdlib(file):
    for url in file:
        abfparseres = abfparse.urlparse(url)
        pyres = urllib.parse.urlparse(url)
        assert isinstance(abfparseres, type(pyres)), f"Type: {type(abfparseres)} vs {type(pyres)}"
        assert abfparseres.scheme == pyres.scheme, f"Scheme: {abfparseres.scheme!r} != {pyres.scheme!r}"
        assert abfparseres.netloc == pyres.netloc, f"Netloc: {abfparseres.netloc!r} != {pyres.netloc!r}"
        assert abfparseres.path == pyres.path, f"Path: {abfparseres.path!r} != {pyres.path!r}"
        assert abfparseres.params == pyres.params, f"Params: {abfparseres.params!r} != {pyres.params!r}"
        assert abfparseres.query == pyres.query, f"Query: {abfparseres.query!r} != {pyres.query!r}"
        assert abfparseres.fragment == pyres.fragment, f"Fragment: {abfparseres.fragment!r} != {pyres.fragment!r}"
        assert abfparseres == pyres, f"ParseResult: {abfparseres!r}"
