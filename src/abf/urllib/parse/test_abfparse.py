import abfparse
import urllib.parse

with open("top100.txt", "r") as f:
    URLS = f.readlines()

for url in URLS:
    res = abfparse.url_parse(url)
    pyres = urllib.parse.urlparse(url)

    # Check type compatibility
    assert isinstance(res, type(pyres)), f"abfparse.url_parse does not return ParseResult: {type(res)}"

    # Compare fields using attribute access, with debug info
    assert res.scheme == pyres.scheme, (
        f"scheme mismatch:\n{res.scheme!r}\n{pyres.scheme!r}\nurl={url!r}"
    )
    assert res.netloc == pyres.netloc, (
        f"netloc mismatch:\n{res.netloc!r}\n{pyres.netloc!r}\nurl={url!r}"
    )
    assert res.path == pyres.path, (
        f"path mismatch:\n{res.path!r}\n{pyres.path!r}\nurl={url!r}"
    )
    assert res.params == pyres.params, (
        f"params mismatch:\n{res.params!r}\n{pyres.params!r}\nurl={url!r}"
    )
    assert res.query == pyres.query, (
        f"query mismatch:\n{res.query!r}\n{pyres.query!r}\nurl={url!r}"
    )
    assert res.fragment == pyres.fragment, (
        f"fragment mismatch:\n{res.fragment!r}\n{pyres.fragment!r}\nurl={url!r}"
    )

    assert res == pyres, (
        f"ParseResult mismatch:\n{res=}\n{pyres=}\nurl={url!r}"
    )

print("All tests passed.")
