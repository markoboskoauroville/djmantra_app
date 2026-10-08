#!/usr/bin/env python3
"""DJ Mantra: check the TIDAL API with your own account.

Logs in with TIDAL's official OAuth 2.0 PKCE flow in your browser, then reads
what DJ Mantra needs: your country, your playlists, a few tracks with genres,
BPM, key and popularity, the genre list, a search, and whether play counts
(trackStatistics) are readable. Writes a Markdown report plus raw JSON
responses to testing/results/, which the cloud session uses to build and test
the app's TIDAL code.

Privacy: the access token stays in memory and is never printed or saved.
Your e-mail address is not written to the report.

Usage:  python3 tools/tidal/tidal_check.py [--client-id ID] [--port 8765]
Only the Python 3 standard library is needed.
"""

import argparse
import base64
import datetime
import hashlib
import http.server
import json
import os
import secrets
import socket
import sys
import threading
import urllib.error
import urllib.parse
import urllib.request
import webbrowser

CLIENT_ID = "Etv8AkwIcduV4SYO"  # public; the client secret is never needed (PKCE)
# Overridable only for the offline test against a mock server (tools/tidal/test_tidal_check.py).
LOGIN_URL = os.environ.get("DJMANTRA_TIDAL_LOGIN_URL", "https://login.tidal.com/authorize")
TOKEN_URL = os.environ.get("DJMANTRA_TIDAL_TOKEN_URL", "https://auth.tidal.com/v1/oauth2/token")
API = os.environ.get("DJMANTRA_TIDAL_API_URL", "https://openapi.tidal.com/v2")
SCOPES = "collection.read entitlements.read playlists.read recommendations.read search.read user.read"
SAMPLE_TRACKS = 8


def pkce_pair():
    verifier = base64.urlsafe_b64encode(secrets.token_bytes(48)).rstrip(b"=").decode()
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b"=").decode()
    return verifier, challenge


def wait_for_code(port, expected_state, timeout=300):
    result = {}

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            url = urllib.parse.urlparse(self.path)
            if url.path != "/tidal-callback":
                self.send_response(404)
                self.end_headers()
                return
            query = dict(urllib.parse.parse_qsl(url.query))
            result.update(query)
            ok = query.get("state") == expected_state and "code" in query
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            msg = ("Logged in to TIDAL. You can close this tab and go back to the terminal."
                   if ok else "Login did not complete: " + query.get("error_description", query.get("error", "unknown error")))
            self.wfile.write(f"<html><body style='font-family:sans-serif'><h2>DJ Mantra</h2><p>{msg}</p></body></html>".encode())

        def log_message(self, *args):
            pass

    # "localhost" can resolve to ::1 (IPv6) on macOS: listen on both stacks when possible.
    class DualStackServer(http.server.HTTPServer):
        address_family = socket.AF_INET6

        def server_bind(self):
            self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
            super().server_bind()

    try:
        server = DualStackServer(("::", port), Handler)
    except OSError:
        server = http.server.HTTPServer(("127.0.0.1", port), Handler)
    server.timeout = 1
    deadline = datetime.datetime.now() + datetime.timedelta(seconds=timeout)
    while "code" not in result and "error" not in result and datetime.datetime.now() < deadline:
        server.handle_request()
    server.server_close()
    if result.get("state") != expected_state:
        raise SystemExit("Login failed: state mismatch or no answer from TIDAL. " + json.dumps(
            {k: v for k, v in result.items() if k in ("error", "error_description")}))
    if "code" not in result:
        raise SystemExit("Login failed: " + result.get("error_description", result.get("error", "timeout")))
    return result["code"]


def post_form(url, data):
    req = urllib.request.Request(url, data=urllib.parse.urlencode(data).encode(),
                                 headers={"Content-Type": "application/x-www-form-urlencoded"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.load(resp)


class Api:
    def __init__(self, token):
        self._token = token
        self.calls = []  # (path, status)

    def get(self, path, params=None):
        url = API + path + ("?" + urllib.parse.urlencode(params, doseq=True) if params else "")
        req = urllib.request.Request(url, headers={
            "Authorization": "Bearer " + self._token,
            "Accept": "application/vnd.api+json",
        })
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                body = json.load(resp)
                self.calls.append((path, resp.status))
                return resp.status, body
        except urllib.error.HTTPError as e:
            try:
                body = json.loads(e.read() or b"{}")
            except ValueError:
                body = {}
            self.calls.append((path, e.code))
            return e.code, body


def included_map(doc):
    return {(r.get("type"), r.get("id")): r for r in doc.get("included", [])}


def rel_ids(resource, name):
    data = (resource.get("relationships", {}).get(name) or {}).get("data") or []
    if isinstance(data, dict):
        data = [data]
    return [(d.get("type"), d.get("id")) for d in data]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--client-id", default=CLIENT_ID)
    ap.add_argument("--port", type=int, default=8765)
    ap.add_argument("--out", default=None, help="results folder (default: testing/results)")
    args = ap.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H%M")
    out_dir = args.out or os.path.join(root, "testing", "results")
    raw_dir = os.path.join(out_dir, f"{stamp}_tidal")
    os.makedirs(raw_dir, exist_ok=True)
    redirect = f"http://localhost:{args.port}/tidal-callback"

    # 1. Log in (PKCE)
    verifier, challenge = pkce_pair()
    state = secrets.token_urlsafe(16)
    login = LOGIN_URL + "?" + urllib.parse.urlencode({
        "response_type": "code", "client_id": args.client_id, "redirect_uri": redirect,
        "scope": SCOPES, "code_challenge_method": "S256", "code_challenge": challenge, "state": state,
    })
    print("Opening the TIDAL login in your browser. If it does not open, visit:\n  " + login)
    threading.Timer(0.5, lambda: webbrowser.open(login)).start()
    code = wait_for_code(args.port, state)
    token = post_form(TOKEN_URL, {
        "client_id": args.client_id, "code": code, "code_verifier": verifier,
        "grant_type": "authorization_code", "redirect_uri": redirect, "scope": SCOPES,
    })
    granted = token.get("scope", "")
    api = Api(token["access_token"])
    del token
    print("Logged in. Reading data...")

    report = []
    def section(title):
        report.append(f"\n## {title}\n")

    def save(name, body):
        with open(os.path.join(raw_dir, name), "w", encoding="utf-8") as f:
            json.dump(body, f, indent=1, ensure_ascii=False)

    # 2. User: country only (no e-mail in the report)
    status, me = api.get("/users/me")
    attrs = (me.get("data") or {}).get("attributes", {})
    country = attrs.get("country") or "US"
    section("Account")
    report.append(f"- /users/me: HTTP {status}; country **{attrs.get('country', '?')}**; "
                  f"attributes available: {', '.join(sorted(attrs.keys())) or 'none'}")
    report.append(f"- Scopes granted: `{granted}`")

    # 3. Playlists
    status, pls = api.get("/playlists", {"filter[owners.id]": "me", "countryCode": country})
    save("playlists.json", pls)
    playlists = pls.get("data", [])
    section("Playlists")
    report.append(f"- /playlists?filter[owners.id]=me: HTTP {status}, {len(playlists)} on the first page"
                  f"{' (more pages)' if (pls.get('links') or {}).get('next') else ''}")
    for p in playlists[:15]:
        a = p.get("attributes", {})
        report.append(f"  - {a.get('name', '?')} ({a.get('numberOfItems', '?')} items)")

    # 4. Tracks of the first non-empty playlist
    tracks = []
    first = next((p for p in playlists if (p.get("attributes", {}).get("numberOfItems") or 0) > 0), None)
    section("Tracks")
    if first:
        status, items = api.get(f"/playlists/{first['id']}/relationships/items",
                                {"countryCode": country})
        save("playlist_items.json", items)
        ids = [d["id"] for d in items.get("data", []) if d.get("type") == "tracks"][:SAMPLE_TRACKS]
        report.append(f"- items of \"{first['attributes'].get('name')}\": HTTP {status}, "
                      f"{len(items.get('data', []))} on the first page")
        if ids:
            status, tdoc = api.get("/tracks", {"filter[id]": ",".join(ids), "countryCode": country,
                                               "include": "artists,albums,genres"})
            save("tracks.json", tdoc)
            inc = included_map(tdoc)
            report.append(f"- /tracks?include=artists,albums,genres: HTTP {status}")
            report.append("\n| Artist – Title | ISRC | BPM | Key | Popularity | Genres (track) | Genres (album) |")
            report.append("|---|---|---|---|---|---|---|")
            for t in tdoc.get("data", []):
                a = t.get("attributes", {})
                artists = ", ".join(inc.get(k, {}).get("attributes", {}).get("name", "?") for k in rel_ids(t, "artists"))
                tg = ", ".join(inc.get(k, {}).get("attributes", {}).get("genreName", k[1]) for k in rel_ids(t, "genres"))
                album_g = []
                for k in rel_ids(t, "albums")[:1]:
                    album = inc.get(k, {})
                    album_g = [inc.get(g, {}).get("attributes", {}).get("genreName", g[1]) for g in rel_ids(album, "genres")]
                key = f"{a.get('key', '')} {a.get('keyScale', '')}".strip()
                report.append(f"| {artists} – {a.get('title', '?')} | {a.get('isrc', '')} | {a.get('bpm', '')} | "
                              f"{key} | {a.get('popularity', '')} | {tg or '–'} | {', '.join(album_g) or '–'} |")
                tracks.append(t)
    else:
        report.append("- No playlist with items found.")

    # 5. Play counts
    section("Play counts (trackStatistics)")
    if tracks:
        status, stats = api.get(f"/trackStatistics/{tracks[0]['id']}", {"countryCode": country})
        save("track_statistics.json", stats)
        sa = (stats.get("data") or {}).get("attributes", {})
        report.append(f"- /trackStatistics/{tracks[0]['id']}: HTTP {status}; "
                      + (f"totalPlaybacks {sa.get('totalPlaybacks')}, uniqueListeners {sa.get('uniqueListeners')}"
                         if sa else "not readable for this track"))
    else:
        report.append("- skipped (no track)")

    # 6. Genres TIDAL lets users pick from
    status, genres = api.get("/genres", {"filter[id]": "USER_SELECTABLE", "locale": "en-US"})
    save("genres.json", genres)
    names = [g.get("attributes", {}).get("genreName", "?") for g in genres.get("data", [])]
    section("Genres")
    report.append(f"- /genres?filter[id]=USER_SELECTABLE: HTTP {status}, {len(names)} genres: {', '.join(names)}")

    # 7. Search
    status, sr = api.get("/searchResults", {"filter[query]": "deep house", "countryCode": country,
                                            "include": "tracks"})
    save("search_deep_house.json", sr)
    section("Search")
    report.append(f"- /searchResults?filter[query]=deep house&include=tracks: HTTP {status}, "
                  f"{len([r for r in sr.get('included', []) if r.get('type') == 'tracks'])} tracks included")

    # 8. Summary of calls
    section("All calls")
    for path, st in api.calls:
        report.append(f"- {st} {path}")

    report_path = os.path.join(out_dir, f"{stamp}_tidal.md")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(f"# TIDAL API check {stamp}\n\nClient ID `{args.client_id}`, redirect `{redirect}`. "
                f"Raw responses in `{os.path.basename(raw_dir)}/` (no tokens, no e-mail).\n")
        f.write("\n".join(report) + "\n")
    print("REPORT=" + os.path.relpath(report_path, root))


if __name__ == "__main__":
    try:
        main()
    except urllib.error.URLError as e:
        sys.exit(f"Network error: {e}")
