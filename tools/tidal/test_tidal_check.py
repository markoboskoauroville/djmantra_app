#!/usr/bin/env python3
"""Offline test of tidal_check.py against a mock TIDAL server (no network, no account).

The mock checks the PKCE parameters, issues a token, and serves JSON:API
responses shaped like TIDAL's API (from the official SDK's generated types).
Run: python3 tools/tidal/test_tidal_check.py
"""
import base64, hashlib, http.server, json, os, subprocess, sys, tempfile, threading, urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
SEEN = {}

def doc(data, included=None, links=None):
    d = {"data": data}
    if included is not None: d["included"] = included
    d["links"] = links or {"self": "/x"}
    return d

TRACKS = [
    {"id": "1001", "type": "tracks", "attributes": {"title": "Shallow Water", "isrc": "DEAA12300001", "bpm": 118.0,
     "key": "A", "keyScale": "MINOR", "popularity": 0.42, "duration": "PT6M12S"},
     "relationships": {"artists": {"data": [{"id": "a1", "type": "artists"}]},
                       "albums": {"data": [{"id": "al1", "type": "albums"}]},
                       "genres": {"data": [{"id": "g1", "type": "genres"}]}}},
    {"id": "1002", "type": "tracks", "attributes": {"title": "Arabesque 3", "isrc": "DEAA12300002", "bpm": 122.0,
     "key": "D", "keyScale": "MAJOR", "popularity": 0.17, "duration": "PT7M01S"},
     "relationships": {"artists": {"data": [{"id": "a2", "type": "artists"}]},
                       "albums": {"data": [{"id": "al1", "type": "albums"}]},
                       "genres": {"data": []}}},
]
INCLUDED = [
    {"id": "a1", "type": "artists", "attributes": {"name": "Björk & Friends"}},
    {"id": "a2", "type": "artists", "attributes": {"name": "Ozan"}},
    {"id": "al1", "type": "albums", "attributes": {"title": "Organic"},
     "relationships": {"genres": {"data": [{"id": "g2", "type": "genres"}]}}},
    {"id": "g1", "type": "genres", "attributes": {"genreName": "organic house"}},
    {"id": "g2", "type": "genres", "attributes": {"genreName": "electronic"}},
]

class Mock(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send_json(self, body, status=200):
        b = json.dumps(body).encode()
        self.send_response(status); self.send_header("Content-Type", "application/vnd.api+json")
        self.send_header("Content-Length", str(len(b))); self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        u = urllib.parse.urlparse(self.path); q = dict(urllib.parse.parse_qsl(u.query))
        if u.path == "/authorize":
            SEEN["authorize"] = q
            loc = q["redirect_uri"] + "?" + urllib.parse.urlencode({"code": "CODE123", "state": q["state"]})
            self.send_response(302); self.send_header("Location", loc); self.end_headers(); return
        if self.headers.get("Authorization") != "Bearer TOKEN-xyz":
            return self.send_json({"errors": [{"status": "401"}]}, 401)
        SEEN.setdefault("api", []).append((u.path, q))
        if u.path == "/v2/users/me":
            return self.send_json(doc({"id": "u1", "type": "users", "attributes": {"country": "HR", "email": "secret@example.com", "username": "m"}}))
        if u.path == "/v2/playlists":
            return self.send_json(doc([{"id": "p0", "type": "playlists", "attributes": {"name": "Empty", "numberOfItems": 0}},
                                       {"id": "p1", "type": "playlists", "attributes": {"name": "Friday set", "numberOfItems": 2}}]))
        if u.path == "/v2/playlists/p1/relationships/items":
            return self.send_json(doc([{"id": "1001", "type": "tracks"}, {"id": "1002", "type": "tracks"}]))
        if u.path == "/v2/tracks":
            ids = q["filter[id]"].split(",")
            return self.send_json(doc([t for t in TRACKS if t["id"] in ids], INCLUDED))
        if u.path == "/v2/trackStatistics/1001":
            return self.send_json({"errors": [{"status": "403", "detail": "Forbidden"}]}, 403)
        if u.path == "/v2/genres":
            return self.send_json(doc([{"id": "g1", "type": "genres", "attributes": {"genreName": "organic house"}},
                                       {"id": "g3", "type": "genres", "attributes": {"genreName": "pop"}}]))
        if u.path == "/v2/searchResults":
            return self.send_json(doc([{"id": "deep house", "type": "searchResults"}], [TRACKS[0]]))
        self.send_json({"errors": [{"status": "404"}]}, 404)
    def do_POST(self):
        body = dict(urllib.parse.parse_qsl(self.rfile.read(int(self.headers["Content-Length"])).decode()))
        SEEN["token"] = body
        a = SEEN["authorize"]
        ok = (body.get("code") == "CODE123" and body.get("grant_type") == "authorization_code"
              and base64.urlsafe_b64encode(hashlib.sha256(body["code_verifier"].encode()).digest()).rstrip(b"=").decode()
                  == a["code_challenge"] and body.get("redirect_uri") == a["redirect_uri"] and "client_secret" not in body)
        if not ok:
            return self.send_json({"error": "invalid_grant"}, 400)
        self.send_json({"access_token": "TOKEN-xyz", "token_type": "Bearer", "expires_in": 3600, "scope": a["scope"]})

def main():
    srv = http.server.HTTPServer(("127.0.0.1", 0), Mock); port = srv.server_port
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    out = tempfile.mkdtemp()
    env = dict(os.environ,
               DJMANTRA_TIDAL_LOGIN_URL=f"http://127.0.0.1:{port}/authorize",
               DJMANTRA_TIDAL_TOKEN_URL=f"http://127.0.0.1:{port}/token",
               DJMANTRA_TIDAL_API_URL=f"http://127.0.0.1:{port}/v2",
               # "Browser": follow the login redirect to the script's callback, like a real browser would.
               BROWSER="curl -sL -o /dev/null %s")
    r = subprocess.run([sys.executable, os.path.join(HERE, "tidal_check.py"), "--port", "8799", "--out", out],
                       env=env, capture_output=True, text=True, timeout=60)
    print(r.stdout[-2000:], r.stderr[-2000:])
    assert r.returncode == 0, "script failed"
    a = SEEN["authorize"]
    assert a["client_id"] == "Etv8AkwIcduV4SYO" and a["code_challenge_method"] == "S256"
    assert a["redirect_uri"] == "http://localhost:8799/tidal-callback"
    assert set(a["scope"].split()) == {"collection.read", "entitlements.read", "playlists.read",
                                       "recommendations.read", "search.read", "user.read"}
    paths = [p for p, _ in SEEN["api"]]
    assert "/v2/playlists/p1/relationships/items" in paths, paths   # skipped the empty playlist
    tq = dict(SEEN["api"])["/v2/tracks"]
    assert tq["include"] == "artists,albums,genres" and tq["countryCode"] == "HR"
    report = open([os.path.join(out, f) for f in os.listdir(out) if f.endswith("_tidal.md")][0], encoding="utf-8").read()
    for expected in ["country **HR**", "Friday set (2 items)", "Björk & Friends – Shallow Water | DEAA12300001 | 118.0 | A MINOR | 0.42 | organic house | electronic",
                     "Ozan – Arabesque 3", "| – | electronic |", "HTTP 403; not readable for this track", "2 genres: organic house, pop",
                     "1 tracks included"]:
        assert expected in report, expected
    everything = report + "".join(open(os.path.join(dp, f), encoding="utf-8").read()
                                  for dp, _, fs in os.walk(out) for f in fs)
    assert "TOKEN-xyz" not in everything and "secret@example.com" not in everything and "CODE123" not in everything
    print("OK: login flow, API calls and report verified; no token, code or e-mail written")

main()
