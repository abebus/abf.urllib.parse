import abfparse
import urllib.parse

from collections import defaultdict

url_errors = defaultdict(list)

for filename in ["top100.txt", "userbait.txt", "wikipedia_100k.txt", "linux_files.txt", "node_files.txt", "kasztp.txt", "isaacs_files.txt"]:
    with open(filename) as f:
        for url in f:
            abfparseres = abfparse.urlparse(url)
            pyres = urllib.parse.urlparse(url)
            errs = []

            if not isinstance(abfparseres, type(pyres)):
                errs.append(f"Type: abfparse.url_parse does not return ParseResult: {type(abfparseres)} vs {type(pyres)}")
            if abfparseres.scheme != pyres.scheme:
                errs.append(f"Scheme: {abfparseres.scheme=!r} != {pyres.scheme=!r}")
            if abfparseres.netloc != pyres.netloc:
                errs.append(f"Netloc: {abfparseres.netloc=!r} != {pyres.netloc=!r}")
            if abfparseres.path != pyres.path:
                errs.append(f"Path: {abfparseres.path=!r} != {pyres.path=!r}")
            if abfparseres.params != pyres.params:
                errs.append(f"Params: {abfparseres.params=!r} != {pyres.params=!r}")
            if abfparseres.query != pyres.query:
                errs.append(f"Query: {abfparseres.query=!r} != {pyres.query=!r}")
            if abfparseres.fragment != pyres.fragment:
                errs.append(f"Fragment: {abfparseres.fragment=!r} != {pyres.fragment=!r}")
            if abfparseres != pyres:
                errs.append(f"ParseResult: {abfparseres=!r} != {pyres=!r}")

            if errs:
                print(f"\nDEBUG: repr(url): {repr(url)}")
                print(f"DEBUG: type(url): {type(url)}")
                print(f"DEBUG: abfparseres.fragment: {repr(abfparseres.fragment)}")
                print(f"DEBUG: pyres.fragment: {repr(pyres.fragment)}")
                url_errors[url.strip()] += errs

if not url_errors:
    print("All tests passed.")
else:
    print(f"Total URLs with errors: {len(url_errors)}")
    for url, errs in url_errors.items():
        print(f"\nURL: {url}")
        print(f"\nURL encoded: {url.encode("ascii")}")
        for err in errs:
            print(f"  - {err}")
