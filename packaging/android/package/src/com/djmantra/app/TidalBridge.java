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
    private static final String CLIENT_ID = "Etv8AkwIcduV4SYO";
    private static final int PORT = 8765;
    private static final String REDIRECT = "http://localhost:" + PORT + "/tidal-callback";
    private static final String SCOPES =
            "user.read search.read playlists.read playlists.write collection.read";
    private static final String API = "https://openapi.tidal.com/v2";
    private static final String PREFS = "djmantra_tidal";

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    public static boolean loggedIn(Context context) {
        return !prefs(context).getString("refresh", "").isEmpty();
    }

    public static void logout(Context context) {
        prefs(context).edit().clear().apply();
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
                    + "&client_id=" + enc(CLIENT_ID) + "&redirect_uri=" + enc(REDIRECT)
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
                                + enc(CLIENT_ID) + "&code=" + enc(code) + "&redirect_uri=" + enc(REDIRECT)
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
                    JSONObject doc = get(context, "/searchResults?filter%5Bquery%5D="
                            + enc(s.optString("artist") + " " + s.optString("title"))
                            + "&include=tracks&countryCode=" + country);
                    JSONArray included = doc.optJSONArray("included");
                    for (int k = 0; included != null && k < included.length() && id == null; ++k) {
                        if ("tracks".equals(included.getJSONObject(k).optString("type"))) {
                            id = included.getJSONObject(k).getString("id");
                        }
                    }
                }
                if (id != null) {
                    found.put(new JSONObject().put("id", id).put("type", "tracks"));
                }
            }
            JSONObject body = new JSONObject().put("data", new JSONObject().put("type", "playlists")
                    .put("attributes", new JSONObject().put("name", name)
                            .put("description", "From DJ Mantra")));
            JSONObject created = send(context, "POST", "/playlists?countryCode=" + country, body);
            String playlistId = created.getJSONObject("data").getString("id");
            for (int start = 0; start < found.length(); start += 20) {
                JSONArray batch = new JSONArray();
                for (int i = start; i < Math.min(found.length(), start + 20); ++i) {
                    batch.put(found.get(i));
                }
                send(context, "POST", "/playlists/" + playlistId + "/relationships/items?countryCode=" + country,
                        new JSONObject().put("data", batch));
            }
            Log.i(TAG, "pushed " + name + ": " + found.length() + "/" + songs.length());
            return found.length() + "/" + songs.length();
        } catch (Exception e) {
            Log.w(TAG, "push: " + e);
            return "error: " + e.getMessage();
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
        JSONObject token = postForm(context, "grant_type=refresh_token&client_id=" + enc(CLIENT_ID)
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

    private static JSONObject send(Context context, String method, String path, JSONObject body) throws Exception {
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
