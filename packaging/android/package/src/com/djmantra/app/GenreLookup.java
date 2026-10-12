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
    public static String lookup(String artist, String title) {
        try {
            String query = "recording:\"" + clean(title) + "\"";
            if (artist != null && !artist.isEmpty()) {
                query += " AND artist:\"" + clean(artist) + "\"";
            }
            JSONObject found = get("recording?fmt=json&limit=5&query="
                    + URLEncoder.encode(query, "UTF-8"));
            JSONArray recordings = found.optJSONArray("recordings");
            if (recordings == null || recordings.length() == 0) {
                Log.i(TAG, "GenreLookup: no recording for " + artist + " - " + title);
                return "";
            }
            JSONObject best = recordings.getJSONObject(0);
            if (best.optInt("score", 0) < 80) {
                Log.i(TAG, "GenreLookup: no close match for " + artist + " - " + title);
                return "";
            }
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
