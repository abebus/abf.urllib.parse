import abfparse
import urllib.parse

url = "http://user:pass@host:80/path;params?query=1#frag"
s = "abc def/!"
res = abfparse.url_parse(url)
print("abfparse.url_parse:", res)

pyres = urllib.parse.urlparse(url)
print("urllib.parse.urlparse:", pyres)

# Check type compatibility
assert isinstance(res, type(pyres)), f"abfparse.url_parse does not return ParseResult: {type(res)}"

# Compare fields using attribute access
assert res.scheme == pyres.scheme
assert res.netloc == pyres.netloc
assert res.path == pyres.path
assert res.params == pyres.params
assert res.query == pyres.query
assert res.fragment == pyres.fragment

s = "abc def/!"
q1 = abfparse.url_quote(s)
q2 = urllib.parse.quote(s)
print("abfparse.url_quote:", q1)
print("urllib.parse.quote:", q2)

# Compare quote results
assert q1 == q2.replace('%2F', '/')  # abfparse keeps '/' unquoted by default
print("All tests passed.")
