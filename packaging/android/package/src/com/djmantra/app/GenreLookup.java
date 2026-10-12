// DJ Mantra: a song's genre from MusicBrainz, when its file has none (Marko,
// 12.10.2026: "if it's not there you need to find with connecting this up to
// the internet, genre of the song").
//
// MusicBrainz is an open music database with an official public web service
// (https://musicbrainz.org/doc/MusicBrainz_API): no key, at most one request
// a second, a User-Agent naming the app. Android's own HTTPS is used (the APK
// has no OpenSSL for Qt's network stack). Called from a worker thread of the
// app, never the UI thread.
package com.djmantra.app;

import android.util.Log;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import org.json.JSONArray;
import org.json.JSONObject;

public final class GenreLookup {
    private GenreLookup() {}

    private static final String TAG = "DJMantra";
    private static final String BASE = "https://musicbrainz.org/ws/2/";
    private static final String AGENT = "DJMantra/2.5 ( https://djmantra.pages.dev )";
    private static long sLastRequest = 0;

    /// The genre ("Deep House"), or "" when MusicBrainz knows none
    public static String lookup(String fullArtist, String fullTitle) {
        String genre = musicBrainz(fullArtist, fullTitle);
        if (genre.isEmpty()) {
            // round 20: MusicBrainz knew 2 of 6 songs; Deezer's public catalogue
            // (official API, no key) knows many more electronic releases
            genre = deezer(firstArtist(fullArtist), baseTitle(fullTitle));
        }
        return genre;
    }

    // Deezer: the track found by artist and title, its album's genres
    static String deezer(String artist, String title) {
        try {
            String query = "track:\"" + clean(title) + "\"";
            if (!artist.isEmpty()) {
                query = "artist:\"" + clean(artist) + "\" " + query;
            }
            // round 22: the advanced query ("artist:... track:...") found
            // nothing 4 times in 4: then the plain words, as a person types
            JSONArray tracks = deezerSearch(query);
            if (tracks == null || tracks.length() == 0) {
                tracks = deezerSearch((artist + " " + title).trim());
            }
            if (tracks == null || tracks.length() == 0) {
                Log.i(TAG, "GenreLookup: Deezer has no track for " + artist + " - " + title);
                return "";
            }
            JSONObject album = tracks.getJSONObject(0).optJSONObject("album");
            if (album == null) {
                return "";
            }
            JSONObject full = getJson("https://api.deezer.com/album/" + album.optLong("id"));
            JSONObject genres = full.optJSONObject("genres");
            JSONArray data = genres != null ? genres.optJSONArray("data") : null;
            String genre = data != null && data.length() > 0
                    ? data.getJSONObject(0).optString("name", "") : "";
            Log.i(TAG, "GenreLookup: Deezer " + artist + " - " + title + ": "
                    + (genre.isEmpty() ? "none" : genre));
            return genre;
        } catch (Throwable error) {
            Log.w(TAG, "GenreLookup: Deezer failed for " + artist + " - " + title + ": " + error);
            return "";
        }
    }

    private static JSONArray deezerSearch(String query) throws Exception {
        String url = "https://api.deezer.com/search?limit=3&q=" + URLEncoder.encode(query, "UTF-8");
        JSONObject found = getJson(url);
        JSONArray data = found.optJSONArray("data");
        Log.i(TAG, "GenreLookup: Deezer " + url + " -> " + (data == null ? "no data: "
                + found.toString().substring(0, Math.min(200, found.toString().length()))
                : data.length() + " tracks"));
        return data;
    }

    private static String musicBrainz(String fullArtist, String fullTitle) {
        // Round 18: "Jakatta, PROFF - American Dream - PROFF Interpretation"
        // was not found: the first artist only, the title without its mix name
        final String artist = firstArtist(fullArtist);
        final String title = baseTitle(fullTitle);
        try {
            String query = "recording:\"" + clean(title) + "\"";
            if (!artist.isEmpty()) {
                query += " AND artist:\"" + clean(artist) + "\"";
            }
            JSONObject found = get("recording?fmt=json&limit=5&query="
                    + URLEncoder.encode(query, "UTF-8"));
            JSONArray recordings = found.optJSONArray("recordings");
            if (recordings == null || recordings.length() == 0
                    || recordings.getJSONObject(0).optInt("score", 0) < 80) {
                // The song is not known: the artist's genre
                String genre = artistGenre(artist);
                Log.i(TAG, "GenreLookup: no recording for " + artist + " - " + title
                        + ", artist's genre: " + (genre.isEmpty() ? "none" : genre));
                return genre;
            }
            JSONObject best = recordings.getJSONObject(0);
            // The recording's own genres, else its artist's
            String genre = topGenre(get("recording/" + best.getString("id")
                    + "?fmt=json&inc=genres"));
            if (genre.isEmpty()) {
                JSONArray credits = best.optJSONArray("artist-credit");
                if (credits != null && credits.length() > 0) {
                    JSONObject credit = credits.getJSONObject(0).optJSONObject("artist");
                    if (credit != null) {
                        genre = topGenre(get("artist/" + credit.getString("id")
                                + "?fmt=json&inc=genres"));
                    }
                }
            }
            Log.i(TAG, "GenreLookup: " + artist + " - " + title + ": "
                    + (genre.isEmpty() ? "none" : genre));
            return genre;
        } catch (Throwable error) {
            Log.w(TAG, "GenreLookup failed for " + artist + " - " + title + ": " + error);
            return "";
        }
    }

    private static String artistGenre(String artist) throws Exception {
        if (artist.isEmpty()) {
            return "";
        }
        JSONObject found = get("artist?fmt=json&limit=1&query="
                + URLEncoder.encode("artist:\"" + clean(artist) + "\"", "UTF-8"));
        JSONArray artists = found.optJSONArray("artists");
        if (artists == null || artists.length() == 0
                || artists.getJSONObject(0).optInt("score", 0) < 90) {
            return "";
        }
        return topGenre(get("artist/" + artists.getJSONObject(0).getString("id")
                + "?fmt=json&inc=genres"));
    }

    // "Eran Hersh, Anorre, The NGHBRS" -> "Eran Hersh"; "A feat. B" -> "A"
    static String firstArtist(String artist) {
        if (artist == null) {
            return "";
        }
        String first = artist.split("(?i)\\s*(,|&|;|\\bfeat\\.?|\\bft\\.|\\bx\\b|\\bvs\\.?)\\s*")[0];
        return first.trim();
    }

    // "American Dream - PROFF Interpretation" -> "American Dream";
    // "Song (Extended Mix)" -> "Song"
    static String baseTitle(String title) {
        if (title == null) {
            return "";
        }
        String base = title.replaceAll("\\s*[\\(\\[].*?[\\)\\]]", "");
        int dash = base.indexOf(" - ");
        if (dash > 0) {
            base = base.substring(0, dash);
        }
        return base.trim();
    }

    // Lucene's special characters out of the query
    private static String clean(String text) {
        return text == null ? "" : text.replaceAll("[\\\\\"+\\-!(){}\\[\\]^~*?:/&|]", " ").trim();
    }

    // The genre with the most votes, in title case
    private static String topGenre(JSONObject entity) {
        JSONArray genres = entity.optJSONArray("genres");
        if (genres == null) {
            return "";
        }
        String best = "";
        int bestCount = -1;
        for (int i = 0; i < genres.length(); ++i) {
            JSONObject genre = genres.optJSONObject(i);
            if (genre != null && genre.optInt("count", 0) > bestCount) {
                bestCount = genre.optInt("count", 0);
                best = genre.optString("name", "");
            }
        }
        StringBuilder titled = new StringBuilder();
        for (String word : best.split(" ")) {
            if (word.isEmpty()) {
                continue;
            }
            if (titled.length() > 0) {
                titled.append(' ');
            }
            titled.append(Character.toUpperCase(word.charAt(0))).append(word.substring(1));
        }
        return titled.toString();
    }

    private static synchronized JSONObject getJson(String url) throws Exception {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        try {
            connection.setRequestProperty("User-Agent", AGENT);
            connection.setRequestProperty("Accept", "application/json");
            connection.setConnectTimeout(8000);
            connection.setReadTimeout(8000);
            int status = connection.getResponseCode();
            if (status != 200) {
                Log.w(TAG, "GenreLookup: HTTP " + status + " for " + url);
                throw new Exception("HTTP " + status);
            }
            InputStream in = connection.getInputStream();
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buffer = new byte[8192];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
            return new JSONObject(out.toString("UTF-8"));
        } finally {
            connection.disconnect();
        }
    }

    private static synchronized JSONObject get(String path) throws Exception {
        // MusicBrainz: at most one request a second
        long wait = sLastRequest + 1100 - System.currentTimeMillis();
        if (wait > 0) {
            Thread.sleep(wait);
        }
        sLastRequest = System.currentTimeMillis();
        HttpURLConnection connection = (HttpURLConnection) new URL(BASE + path).openConnection();
        try {
            connection.setRequestProperty("User-Agent", AGENT);
            connection.setRequestProperty("Accept", "application/json");
            connection.setConnectTimeout(8000);
            connection.setReadTimeout(8000);
            int status = connection.getResponseCode();
            if (status == 503) {
                // MusicBrainz is busy (round 18): once more after a pause
                connection.disconnect();
                Thread.sleep(2500);
                sLastRequest = System.currentTimeMillis();
                connection = (HttpURLConnection) new URL(BASE + path).openConnection();
                connection.setRequestProperty("User-Agent", AGENT);
                connection.setRequestProperty("Accept", "application/json");
                connection.setConnectTimeout(8000);
                connection.setReadTimeout(8000);
                status = connection.getResponseCode();
            }
            if (status != 200) {
                throw new Exception("HTTP " + status);
            }
            InputStream in = connection.getInputStream();
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buffer = new byte[8192];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
            return new JSONObject(out.toString("UTF-8"));
        } finally {
            connection.disconnect();
        }
    }
}
