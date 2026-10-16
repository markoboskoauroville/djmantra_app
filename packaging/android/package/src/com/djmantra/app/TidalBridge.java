package com.djmantra.app;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.util.Base64;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.SecureRandom;

/**
 * TIDAL playlists both ways (Marko 14.10.2026): the official TIDAL API v2 (openapi.tidal.com)
 * with OAuth 2.0 PKCE (public Client ID, no secret; the redirect to http://localhost:8765 is
 * caught by a small server here). Tokens stay in the app's private preferences, never anywhere
 * else. Every call blocks: QmlDJMantra runs them off the UI thread.
 */
public final class TidalBridge {
    private static final String TAG = "DJMantraTidal";
    // the DJ Mantra app on Marko's TIDAL developer account (14.10.2026, public); a key file picked
    // in Settings -> TIDAL can set another one (only its Client ID is read, never the secret:
    // PKCE needs none)
    private static final String DEFAULT_CLIENT_ID = "JjTAmDVohiV7GgOL";
    private static final int PORT = 8765;
    private static final String REDIRECT = "http://localhost:" + PORT + "/tidal-callback";
    private static final String SCOPES =
            "user.read search.read playlists.read playlists.write collection.read";
    private static final String API = "https://openapi.tidal.com/v2";
    private static final String PREFS = "djmantra_tidal";

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    public static String clientId(Context context) {
        return prefs(context).getString("client_id", DEFAULT_CLIENT_ID);
    }

    /** Another Client ID (from a key file): the old login is dropped. */
    public static void setClientId(Context context, String id) {
        String clean = id == null ? "" : id.trim();
        prefs(context).edit().clear().putString("client_id", clean.isEmpty() ? DEFAULT_CLIENT_ID : clean).apply();
        Log.i(TAG, "client ID set (" + clean.length() + " characters)");
    }

    public static boolean loggedIn(Context context) {
        return !prefs(context).getString("refresh", "").isEmpty();
    }

    public static void logout(Context context) {
        String id = clientId(context);
        prefs(context).edit().clear().putString("client_id", id).apply();
    }

    // --- login ------------------------------------------------------------------

    /** Opens TIDAL's login in the browser and waits (up to 5 min) for the redirect. "" = ok. */
    public static String login(Context context) {
        try {
            byte[] random = new byte[48];
            new SecureRandom().nextBytes(random);
            String verifier = b64(random);
            String challenge = b64(MessageDigest.getInstance("SHA-256")
                    .digest(verifier.getBytes(StandardCharsets.US_ASCII)));
            byte[] stateBytes = new byte[16];
            new SecureRandom().nextBytes(stateBytes);
            String state = b64(stateBytes);
            String url = "https://login.tidal.com/authorize?response_type=code"
                    + "&client_id=" + enc(clientId(context)) + "&redirect_uri=" + enc(REDIRECT)
                    + "&scope=" + enc(SCOPES) + "&code_challenge_method=S256"
                    + "&code_challenge=" + enc(challenge) + "&state=" + enc(state);
            try (ServerSocket server = new ServerSocket(PORT, 1, InetAddress.getByName("127.0.0.1"))) {
                server.setSoTimeout(300000);
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
                while (true) {
                    try (Socket socket = server.accept()) {
                        String request = readRequestLine(socket.getInputStream());
                        Uri uri = Uri.parse("http://localhost" + request.split(" ")[1]);
                        if (!"/tidal-callback".equals(uri.getPath())) {
                            reply(socket, "Not here.");
                            continue;
                        }
                        String code = uri.getQueryParameter("code");
                        if (code == null || !state.equals(uri.getQueryParameter("state"))) {
                            String error = uri.getQueryParameter("error_description");
                            reply(socket, "TIDAL login did not complete. Go back to DJ Mantra.");
                            return error != null ? error : "login cancelled";
                        }
                        reply(socket, "TIDAL is connected. Go back to DJ Mantra.");
                        JSONObject token = postForm(context, "grant_type=authorization_code&client_id="
                                + enc(clientId(context)) + "&code=" + enc(code) + "&redirect_uri=" + enc(REDIRECT)
                                + "&code_verifier=" + enc(verifier) + "&scope=" + enc(SCOPES));
                        if (token == null) {
                            return "the token exchange failed";
                        }
                        Log.i(TAG, "logged in, scopes: " + token.optString("scope"));
                        return "";
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "login: " + e);
            return String.valueOf(e.getMessage());
        }
    }

    /** The scopes TIDAL granted (playlists.write is needed to send playlists). */
    public static String scopes(Context context) {
        return prefs(context).getString("scope", "");
    }

    // --- playlists ---------------------------------------------------------------

    /** The user's TIDAL playlists: [{id, name, count}] as JSON. */
    public static String playlists(Context context) {
        try {
            String country = country(context);
            JSONArray result = new JSONArray();
            String next = "/playlists?filter%5Bowners.id%5D=me&countryCode=" + country;
            for (int page = 0; next != null && page < 20; ++page) {
                JSONObject doc = get(context, next);
                JSONArray data = doc.optJSONArray("data");
                for (int i = 0; data != null && i < data.length(); ++i) {
                    JSONObject p = data.getJSONObject(i);
                    JSONObject a = p.optJSONObject("attributes");
                    result.put(new JSONObject().put("id", p.getString("id"))
                            .put("name", a != null ? a.optString("name") : "")
                            .put("count", a != null ? a.optInt("numberOfItems") : 0));
                }
                next = nextLink(doc);
            }
            return result.toString();
        } catch (Exception e) {
            Log.w(TAG, "playlists: " + e);
            return "[]";
        }
    }

    /** A TIDAL playlist's tracks in order: [{artist, title, isrc}] as JSON. */
    public static String playlistTracks(Context context, String id) {
        try {
            String country = country(context);
            JSONArray ids = new JSONArray();
            String next = "/playlists/" + id + "/relationships/items?countryCode=" + country;
            for (int page = 0; next != null && page < 100; ++page) {
                JSONObject doc = get(context, next);
                JSONArray data = doc.optJSONArray("data");
                for (int i = 0; data != null && i < data.length(); ++i) {
                    JSONObject item = data.getJSONObject(i);
                    if ("tracks".equals(item.optString("type"))) {
                        ids.put(item.getString("id"));
                    }
                }
                next = nextLink(doc);
            }
            JSONArray result = new JSONArray();
            for (int start = 0; start < ids.length(); start += 20) {
                StringBuilder batch = new StringBuilder();
                for (int i = start; i < Math.min(ids.length(), start + 20); ++i) {
                    batch.append(i > start ? "," : "").append(ids.getString(i));
                }
                JSONObject doc = get(context, "/tracks?filter%5Bid%5D=" + enc(batch.toString())
                        + "&include=artists&countryCode=" + country);
                JSONObject artists = new JSONObject();
                JSONArray included = doc.optJSONArray("included");
                for (int i = 0; included != null && i < included.length(); ++i) {
                    JSONObject r = included.getJSONObject(i);
                    if ("artists".equals(r.optString("type"))) {
                        artists.put(r.getString("id"), r.optJSONObject("attributes").optString("name"));
                    }
                }
                // in the playlist's order
                JSONObject byId = new JSONObject();
                JSONArray data = doc.optJSONArray("data");
                for (int i = 0; data != null && i < data.length(); ++i) {
                    JSONObject t = data.getJSONObject(i);
                    StringBuilder names = new StringBuilder();
                    JSONObject rel = t.optJSONObject("relationships");
                    JSONArray ar = rel != null && rel.optJSONObject("artists") != null
                            ? rel.optJSONObject("artists").optJSONArray("data") : null;
                    for (int k = 0; ar != null && k < ar.length(); ++k) {
                        names.append(k > 0 ? ", " : "").append(artists.optString(ar.getJSONObject(k).getString("id")));
                    }
                    JSONObject a = t.optJSONObject("attributes");
                    byId.put(t.getString("id"), new JSONObject().put("artist", names.toString())
                            .put("title", a != null ? a.optString("title") : "")
                            .put("isrc", a != null ? a.optString("isrc") : ""));
                }
                for (int i = start; i < Math.min(ids.length(), start + 20); ++i) {
                    JSONObject t = byId.optJSONObject(ids.getString(i));
                    if (t != null) {
                        result.put(t);
                    }
                }
            }
            return result.toString();
        } catch (Exception e) {
            Log.w(TAG, "playlistTracks: " + e);
            return "[]";
        }
    }

    /**
     * Makes a TIDAL playlist `name` from songs [{artist, title, isrc}] (JSON): each found by ISRC
     * when there is one, else by "artist title" (the first track). Returns "found/total" or an error.
     */
    public static String pushPlaylist(Context context, String name, String songsJson) {
        sLastPlaylistId = null;
        try {
            if (!scopes(context).contains("playlists.write")) {
                return "TIDAL did not allow writing playlists (playlists.write): log in again after it is enabled";
            }
            String country = country(context);
            JSONArray songs = new JSONArray(songsJson);
            JSONArray found = new JSONArray();
            for (int i = 0; i < songs.length(); ++i) {
                JSONObject s = songs.getJSONObject(i);
                String id = null;
                if (!s.optString("isrc").isEmpty()) {
                    JSONArray data = get(context, "/tracks?filter%5Bisrc%5D=" + enc(s.optString("isrc"))
                            + "&countryCode=" + country).optJSONArray("data");
                    id = data != null && data.length() > 0 ? data.getJSONObject(0).getString("id") : null;
                }
                if (id == null) {
                    // V86: the search's best MATCH, not its first result (tested 16.10.2026: the first
                    // result gave The Bangles for DSF's "Not Like You", karaoke for "Baby Got Back")
                    id = findTrack(context, s.optString("artist"), s.optString("title"), country);
                    if (id == null) {
                        Log.i(TAG, "not on TIDAL: " + s.optString("artist") + " - " + s.optString("title"));
                    }
                }
                if (id != null) {
                    found.put(new JSONObject().put("id", id).put("type", "tracks"));
                }
            }
            // a TIDAL playlist of that name already (an earlier sync): only the songs not in it yet
            String playlistId = null;
            int already = 0;
            JSONArray mine = new JSONArray(playlists(context));
            for (int i = 0; i < mine.length() && playlistId == null; ++i) {
                if (name.equals(mine.getJSONObject(i).optString("name"))) {
                    playlistId = mine.getJSONObject(i).getString("id");
                }
            }
            if (playlistId != null) {
                java.util.Set<String> there = itemIds(context, playlistId, country);
                JSONArray fresh = new JSONArray();
                for (int i = 0; i < found.length(); ++i) {
                    if (!there.contains(found.getJSONObject(i).getString("id"))) {
                        fresh.put(found.get(i));
                    }
                }
                Log.i(TAG, name + ": on TIDAL already, " + fresh.length() + " new songs");
                already = found.length() - fresh.length();
                found = fresh;
                if (fresh.length() == 0) {
                    sLastPlaylistId = playlistId;
                    return already + "/" + songs.length();
                }
            } else {
                JSONObject body = new JSONObject().put("data", new JSONObject().put("type", "playlists")
                        .put("attributes", new JSONObject().put("name", name)
                                .put("description", "From DJ Mantra")
                                // as the Playlist Generator's: not public, reachable by its link
                                .put("accessType", "UNLISTED")));
                JSONObject created = send(context, "POST", "/playlists?countryCode=" + country, body);
                playlistId = created.getJSONObject("data").getString("id");
            }
            for (int start = 0; start < found.length(); start += 20) {
                JSONArray batch = new JSONArray();
                for (int i = start; i < Math.min(found.length(), start + 20); ++i) {
                    batch.put(found.get(i));
                }
                send(context, "POST", "/playlists/" + playlistId + "/relationships/items?countryCode=" + country,
                        new JSONObject().put("data", batch));
            }
            sLastPlaylistId = playlistId;
            Log.i(TAG, "pushed " + name + ": " + found.length() + "/" + songs.length());
            return (already + found.length()) + "/" + songs.length();
        } catch (Exception e) {
            Log.w(TAG, "push: " + e);
            return "error: " + e.getMessage();
        }
    }

    // --- finding a song on TIDAL without its ISRC (V87) ------------------------------------------
    // Ported from the Playlist Generator (markoboskoauroville/generated_playlists, tools/tidal_playlist.py,
    // tested on 144 songs, 16.10.2026). Marko's rule there: "the song matters, not the exact version":
    // the named remix if TIDAL has it, else the original, else another version of the same song; never
    // another song by the same artist or the same title by someone else.
    private static final java.util.regex.Pattern VARIANT = java.util.regex.Pattern.compile(
            "\\b(remix|mix|mixed|live|edit|version|acoustic|instrumental|dub|karaoke|cover|rework|re-work|reprise"
                    + "|recut|demo|vip)\\b", java.util.regex.Pattern.CASE_INSENSITIVE);
    // never the song (kept out, not only ranked lower)
    private static final String[] NOT_THE_SONG = {"karaoke", "made famous", "tribute", "originally performed",
            "in the style of"};
    private static volatile String sLastPlaylistId;

    private static String ascii(String s) {
        return java.text.Normalizer.normalize(s == null ? "" : s, java.text.Normalizer.Form.NFKD)
                .replaceAll("[^\\p{ASCII}]", "").toLowerCase(java.util.Locale.ROOT);
    }

    // words of a title / name without "(...)", "[...]", "feat. ..."
    static String norm(String s) {
        String t = ascii(s).replaceAll("\\(.*?\\)|\\[.*?\\]|feat\\..*|ft\\..*", " ");
        return t.replaceAll("[^a-z0-9]+", " ").trim().replaceAll(" +", " ");
    }

    private static java.util.Set<String> set(String normed) {
        java.util.Set<String> set = new java.util.HashSet<>(java.util.Arrays.asList(normed.split(" ")));
        set.remove("");
        return set;
    }

    // all the words, versions included
    private static java.util.Set<String> words(String s) {
        return set(ascii(s).replaceAll("[^a-z0-9]+", " ").trim());
    }

    private static double overlap(java.util.Set<String> a, java.util.Set<String> b) {
        java.util.Set<String> both = new java.util.HashSet<>(a);
        both.retainAll(b);
        java.util.Set<String> any = new java.util.HashSet<>(a);
        any.addAll(b);
        return (double) both.size() / Math.max(any.size(), 1);
    }

    static double sim(String a, String b) {
        return overlap(set(norm(a)), set(norm(b)));
    }

    private static boolean hasVersion(String title) {
        return title.contains("(") || title.contains("[");
    }

    static double score(String artist, String title, String name, String version, java.util.List<String> artists) {
        String got = name + " " + (version == null ? "" : version);
        double bestArtist = 0;
        for (String a : artists) {
            bestArtist = Math.max(bestArtist, sim(artist, a));
        }
        double s = 0.6 * sim(title, name) + 0.4 * bestArtist;
        if (hasVersion(title)) { // a named remix / version: its words count too
            s += 0.1 * overlap(words(title), words(got));
        }
        if (((version != null && !version.isEmpty()) || hasVersion(name)) && !hasVersion(title)) {
            s -= 0.02; // a version nobody asked for: the plain one wins a tie
        }
        if (VARIANT.matcher(got).find() && !VARIANT.matcher(title).find()) {
            s -= 0.05; // the original over remixes / live takes nobody asked for
        }
        if (got.toLowerCase(java.util.Locale.ROOT).matches(".*\\bmixed\\b.*")
                && !title.toLowerCase(java.util.Locale.ROOT).matches(".*\\bmixed\\b.*")) {
            s -= 0.05; // a cut from a continuous DJ mix
        }
        return s;
    }

    // the searches, from exact to loose (the looser ones find the original when the remix is missing)
    static java.util.List<String> queries(String artist, String title) {
        String bare = norm(title);
        String first = artist.split(",")[0].split("&")[0].trim();
        String squashed = title.replaceAll("\\(.*?\\)|\\[.*?\\]", " ").replaceAll("[^\\w\\s]", "");
        String plain = title.replaceAll("[()\\[\\]]", " ");
        java.util.List<String> out = new java.util.ArrayList<>();
        for (String q : new String[] {artist + " " + plain, artist + " " + bare, first + " " + bare,
                first + " " + squashed, bare}) {
            q = q.trim().replaceAll("\\s+", " ");
            boolean seen = false;
            for (String o : out) {
                seen |= o.equalsIgnoreCase(q);
            }
            if (!q.isEmpty() && !seen) {
                out.add(q);
            }
        }
        return out;
    }

    /** The TIDAL track that is the song, or null (under 60 %: better nothing than another song). */
    static String findTrack(Context context, String artist, String title, String country) throws Exception {
        double good = hasVersion(title) ? 1.1 : 1.0; // a perfect match stops the search early
        String best = null;
        double bestScore = 0;
        java.util.Set<String> seen = new java.util.HashSet<>();
        for (String q : queries(artist, title)) {
            JSONObject doc = get(context, "/searchResults?filter%5Bquery%5D=" + enc(q)
                    + "&include=tracks,tracks.artists&countryCode=" + country);
            JSONArray included = doc.optJSONArray("included");
            if (included == null) {
                continue;
            }
            java.util.Map<String, String> names = new java.util.HashMap<>();
            for (int i = 0; i < included.length(); ++i) {
                JSONObject r = included.getJSONObject(i);
                if ("artists".equals(r.optString("type"))) {
                    names.put(r.getString("id"), r.optJSONObject("attributes").optString("name"));
                }
            }
            for (int i = 0; i < included.length(); ++i) {
                JSONObject t = included.getJSONObject(i);
                if (!"tracks".equals(t.optString("type")) || !seen.add(t.getString("id"))) {
                    continue;
                }
                JSONObject a = t.optJSONObject("attributes");
                String name = a.optString("title");
                String version = a.isNull("version") ? "" : a.optString("version");
                java.util.List<String> artists = new java.util.ArrayList<>();
                JSONObject rel = t.optJSONObject("relationships");
                JSONArray ar = rel != null && rel.optJSONObject("artists") != null
                        ? rel.optJSONObject("artists").optJSONArray("data") : null;
                for (int k = 0; ar != null && k < ar.length(); ++k) {
                    artists.add(names.getOrDefault(ar.getJSONObject(k).getString("id"), ""));
                }
                double bestArtist = 0;
                for (String n : artists) {
                    bestArtist = Math.max(bestArtist, sim(artist, n));
                }
                if (sim(title, name) < 0.5 || bestArtist < 0.3) {
                    continue; // another song by the same artist, or the same title by someone else
                }
                String all = ascii(name + " " + version);
                boolean notTheSong = false;
                for (String bad : NOT_THE_SONG) {
                    notTheSong |= all.contains(bad) && !ascii(title).contains(bad);
                }
                if (notTheSong) {
                    continue;
                }
                double sc = score(artist, title, name, version, artists);
                if (sc > bestScore) {
                    bestScore = sc;
                    best = t.getString("id");
                }
            }
            if (bestScore >= good - 1e-9) {
                break;
            }
        }
        return bestScore >= 0.6 ? best : null;
    }

    /**
     * BeatStats -> TIDAL -> the app (V87, Marko 16.10.2026: "go through beatstats, scrap few of the
     * playlists, and play them through the dj app"): the songs into TIDAL playlist `name` (made or
     * topped up), then that playlist's tracks for the app's import. JSON {result, id, tracks}.
     */
    public static String pushChart(Context context, String name, String songsJson) {
        try {
            String result = pushPlaylist(context, name, songsJson);
            String id = sLastPlaylistId;
            JSONObject out = new JSONObject().put("result", result).put("id", id == null ? "" : id);
            out.put("tracks", new JSONArray(id == null ? "[]" : playlistTracks(context, id)));
            return out.toString();
        } catch (Exception e) {
            Log.w(TAG, "pushChart: " + e);
            return "{\"result\":\"error: " + String.valueOf(e.getMessage()).replace("\"", "'") + "\",\"tracks\":[]}";
        }
    }

    /**
     * Logs in with a TIDAL refresh token (V87, Marko 16.10.2026: from a file picked in Settings): kept
     * like the one a browser login gives. "" when TIDAL took it, else why not. The token is never logged.
     */
    public static String useRefreshToken(Context context, String token) {
        try {
            token = token == null ? "" : token.trim();
            if (token.length() < 20) {
                return "no refresh token in that file";
            }
            JSONObject t = postForm(context, "grant_type=refresh_token&client_id=" + enc(clientId(context))
                    + "&refresh_token=" + enc(token));
            if (t == null) {
                return "TIDAL did not take this token (another app's, or logged out since)";
            }
            SharedPreferences.Editor e = prefs(context).edit().remove("country");
            if (t.optString("refresh_token").isEmpty()) {
                e.putString("refresh", token);
            }
            e.apply();
            Log.i(TAG, "logged in with a refresh token, scopes: " + t.optString("scope"));
            return "";
        } catch (Exception e) {
            Log.w(TAG, "refresh token: " + e);
            return "error: " + e.getMessage();
        }
    }

    // the track IDs a TIDAL playlist holds
    private static java.util.Set<String> itemIds(Context context, String id, String country) throws Exception {
        java.util.Set<String> ids = new java.util.HashSet<>();
        String next = "/playlists/" + id + "/relationships/items?countryCode=" + country;
        for (int page = 0; next != null && page < 100; ++page) {
            JSONObject doc = get(context, next);
            JSONArray data = doc.optJSONArray("data");
            for (int i = 0; data != null && i < data.length(); ++i) {
                ids.add(data.getJSONObject(i).getString("id"));
            }
            next = nextLink(doc);
        }
        return ids;
    }

    /** The TIDAL track ID of recording `isrc` (exact), "" if none or not logged in. Blocking. */
    public static String trackByIsrc(Context context, String isrc) {
        if (isrc == null || isrc.isEmpty() || !loggedIn(context)) {
            return "";
        }
        try {
            JSONArray data = get(context, "/tracks?filter%5Bisrc%5D=" + enc(isrc)
                    + "&countryCode=" + country(context)).optJSONArray("data");
            return data != null && data.length() > 0 ? data.getJSONObject(0).getString("id") : "";
        } catch (Exception e) {
            Log.w(TAG, "isrc " + isrc + ": " + e);
            return "";
        }
    }

    // --- HTTP ---------------------------------------------------------------------

    private static String country(Context context) throws Exception {
        String country = prefs(context).getString("country", "");
        if (country.isEmpty()) {
            JSONObject me = get(context, "/users/me");
            country = me.getJSONObject("data").getJSONObject("attributes").optString("country", "US");
            prefs(context).edit().putString("country", country).apply();
        }
        return country;
    }

    private static String nextLink(JSONObject doc) {
        JSONObject links = doc.optJSONObject("links");
        String next = links != null ? links.optString("next", "") : "";
        if (next.isEmpty()) {
            return null;
        }
        return next.startsWith("http") ? next.substring(next.indexOf("/v2") + 3) : next;
    }

    private static String accessToken(Context context) throws Exception {
        SharedPreferences p = prefs(context);
        if (System.currentTimeMillis() < p.getLong("expires", 0) - 60000) {
            return p.getString("access", "");
        }
        String refresh = p.getString("refresh", "");
        if (refresh.isEmpty()) {
            throw new Exception("not logged in to TIDAL");
        }
        JSONObject token = postForm(context, "grant_type=refresh_token&client_id=" + enc(clientId(context))
                + "&refresh_token=" + enc(refresh));
        if (token == null) {
            throw new Exception("TIDAL login expired: log in again");
        }
        return p.getString("access", "");
    }

    private static JSONObject postForm(Context context, String form) throws Exception {
        HttpURLConnection c = (HttpURLConnection) new URL("https://auth.tidal.com/v1/oauth2/token").openConnection();
        c.setRequestMethod("POST");
        c.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
        c.setDoOutput(true);
        try (OutputStream out = c.getOutputStream()) {
            out.write(form.getBytes(StandardCharsets.UTF_8));
        }
        int code = c.getResponseCode();
        String body = read(code < 400 ? c.getInputStream() : c.getErrorStream());
        if (code >= 400) {
            Log.w(TAG, "token " + code + ": " + body);
            return null;
        }
        JSONObject token = new JSONObject(body);
        SharedPreferences.Editor e = prefs(context).edit()
                .putString("access", token.optString("access_token"))
                .putLong("expires", System.currentTimeMillis() + token.optLong("expires_in", 3600) * 1000);
        if (!token.optString("refresh_token").isEmpty()) {
            e.putString("refresh", token.optString("refresh_token"));
        }
        if (!token.optString("scope").isEmpty()) {
            e.putString("scope", token.optString("scope"));
        }
        e.apply();
        return token;
    }

    private static JSONObject get(Context context, String path) throws Exception {
        return send(context, "GET", path, null);
    }

    // V86 (tested 16.10.2026 with Marko's account): TIDAL answers 429 after ~7 quick requests; a
    // playlist send then failed ("TIDAL 429") or, with a song looked up by search instead of its
    // ISRC, took another version of it (a double on the next sync). A 429 waits as TIDAL says
    // (Retry-After) or 1, 2, 4 ... s, up to 6 tries. Blocking: off the UI thread, as every call here.
    private static JSONObject send(Context context, String method, String path, JSONObject body) throws Exception {
        for (int attempt = 0;; ++attempt) {
            try {
                return sendOnce(context, method, path, body);
            } catch (RateLimited e) {
                if (attempt >= 5) {
                    throw new Exception("TIDAL 429 (too many requests) on " + path.split("\\?")[0]);
                }
                long wait = e.seconds > 0 ? Math.min(30, e.seconds) * 1000L : (1000L << attempt);
                Log.i(TAG, "TIDAL 429: waiting " + wait + " ms (" + method + " " + path.split("\\?")[0] + ")");
                Thread.sleep(wait);
            }
        }
    }

    private static final class RateLimited extends Exception {
        final long seconds;

        RateLimited(long seconds) {
            this.seconds = seconds;
        }
    }

    private static JSONObject sendOnce(Context context, String method, String path, JSONObject body) throws Exception {
        HttpURLConnection c = (HttpURLConnection) new URL(API + path).openConnection();
        c.setRequestMethod(method);
        c.setRequestProperty("Authorization", "Bearer " + accessToken(context));
        c.setRequestProperty("Accept", "application/vnd.api+json");
        c.setConnectTimeout(20000);
        c.setReadTimeout(60000);
        if (body != null) {
            c.setRequestProperty("Content-Type", "application/vnd.api+json");
            c.setDoOutput(true);
            try (OutputStream out = c.getOutputStream()) {
                out.write(body.toString().getBytes(StandardCharsets.UTF_8));
            }
        }
        int code = c.getResponseCode();
        if (code == 429) {
            long seconds = 0;
            try {
                seconds = Long.parseLong(String.valueOf(c.getHeaderField("Retry-After")).trim());
            } catch (NumberFormatException e) {
                // no number: the doubling wait
            }
            read(c.getErrorStream());
            throw new RateLimited(seconds);
        }
        String text = read(code < 400 ? c.getInputStream() : c.getErrorStream());
        if (code >= 400) {
            Log.w(TAG, method + " " + path + " " + code + ": " + text);
            throw new Exception("TIDAL " + code + " on " + path.split("\\?")[0]);
        }
        return text.isEmpty() ? new JSONObject() : new JSONObject(text);
    }

    private static String read(InputStream in) throws Exception {
        if (in == null) {
            return "";
        }
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        byte[] buffer = new byte[8192];
        int n;
        while ((n = in.read(buffer)) > 0) {
            bytes.write(buffer, 0, n);
        }
        return bytes.toString("UTF-8");
    }

    private static String readRequestLine(InputStream in) throws Exception {
        StringBuilder line = new StringBuilder();
        int ch;
        while ((ch = in.read()) >= 0 && ch != '\n') {
            if (ch != '\r') {
                line.append((char) ch);
            }
        }
        return line.toString();
    }

    private static void reply(Socket socket, String text) throws Exception {
        String html = "<html><body style='font-family:sans-serif;font-size:22px;padding:40px'>" + text + "</body></html>";
        OutputStream out = socket.getOutputStream();
        out.write(("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n" + html)
                .getBytes(StandardCharsets.UTF_8));
        out.flush();
    }

    private static String enc(String s) throws Exception {
        return URLEncoder.encode(s, "UTF-8");
    }

    private static String b64(byte[] bytes) {
        return Base64.encodeToString(bytes, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);
    }
}
