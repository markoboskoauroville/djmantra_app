package com.djmantra.app;

import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * SUNO PLAYLISTS (Marko 14.10.2026): a pasted Suno song link (suno.com/song/<id>, or a short
 * suno.com/s/<code> link) into a file the decks can play. The song's public page is read as any
 * link preview reads it (its Open Graph tags: og:title, og:audio), no Suno API; the MP3 it names
 * is saved in `dir` as "<title>.mp3". Returns "path|title", "noaudio|title" (the page names no
 * audio for a visitor: the song's title only), or "error: ...".
 */
public final class SunoBridge {
    private static final String TAG = "DJMantraSuno";
    private static final String AGENT = "Mozilla/5.0 (Linux; Android 16; Pixel 7) AppleWebKit/537.36 "
            + "(KHTML, like Gecko) Chrome/140.0 Mobile Safari/537.36";

    private SunoBridge() {
    }

    public static String fetch(String link, String dir) {
        try {
            String page = read(link);
            String audio = meta(page, "og:audio");
            if (audio.isEmpty()) {
                audio = meta(page, "og:audio:url");
            }
            if (audio.isEmpty()) {
                // the page names its MP3 in its own data when the tag is missing
                Matcher m = Pattern.compile("https://cdn\\d*\\.suno\\.ai/[A-Za-z0-9-]+\\.mp3").matcher(page);
                audio = m.find() ? m.group() : "";
            }
            String title = meta(page, "og:title").replaceAll("\\s*\\|\\s*Suno\\s*$", "").trim();
            if (audio.isEmpty() || audio.contains("/forbidden")) {
                // Suno gives the audio only to its logged-in owner (14.10.2026: "audio_url" is
                // ".../api/forbidden" for a song shared by link): the title is kept, the song
                // comes by GET (Suno's own download, logged in) or a file matched by its title
                Log.i(TAG, link + ": title only (" + title + ")");
                return title.isEmpty() ? "error: no song on " + link : "noaudio|" + title;
            }
            if (title.isEmpty()) {
                title = audio.substring(audio.lastIndexOf('/') + 1).replace(".mp3", "");
            }
            File folder = new File(dir);
            folder.mkdirs();
            File file = new File(folder, title.replaceAll("[\\\\/:*?\"<>|]", "_") + ".mp3");
            if (file.exists() && file.length() > 0) {
                return file.getAbsolutePath() + "|" + title;
            }
            HttpURLConnection c = open(audio);
            File part = new File(folder, file.getName() + ".part");
            try (InputStream in = c.getInputStream(); FileOutputStream out = new FileOutputStream(part)) {
                byte[] buffer = new byte[65536];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            }
            if (!part.renameTo(file)) {
                return "error: could not save " + file;
            }
            Log.i(TAG, link + " -> " + file + " (" + file.length() + " bytes)");
            return file.getAbsolutePath() + "|" + title;
        } catch (Exception e) {
            Log.w(TAG, link + ": " + e);
            return "error: " + e.getMessage();
        }
    }

    private static HttpURLConnection open(String url) throws Exception {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setInstanceFollowRedirects(true);
        c.setRequestProperty("User-Agent", AGENT);
        c.setConnectTimeout(15000);
        c.setReadTimeout(30000);
        int code = c.getResponseCode();
        if (code >= 300 && code < 400) {
            // http -> https or another host: followed by hand
            return open(new URL(new URL(url), c.getHeaderField("Location")).toString());
        }
        if (code >= 400) {
            throw new Exception("HTTP " + code + " for " + url);
        }
        return c;
    }

    private static String read(String url) throws Exception {
        HttpURLConnection c = open(url);
        try (InputStream in = c.getInputStream()) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            byte[] buffer = new byte[16384];
            int n;
            while ((n = in.read(buffer)) > 0) {
                bytes.write(buffer, 0, n);
            }
            return bytes.toString("UTF-8");
        }
    }

    // <meta property="og:x" content="..."> in either attribute order
    private static String meta(String page, String property) {
        String p = Pattern.quote(property);
        Matcher m = Pattern.compile("<meta[^>]+(?:property|name)=\"" + p + "\"[^>]+content=\"([^\"]*)\"").matcher(page);
        if (m.find()) {
            return unescape(m.group(1));
        }
        m = Pattern.compile("<meta[^>]+content=\"([^\"]*)\"[^>]+(?:property|name)=\"" + p + "\"").matcher(page);
        return m.find() ? unescape(m.group(1)) : "";
    }

    private static String unescape(String s) {
        return s.replace("&amp;", "&").replace("&quot;", "\"").replace("&#x27;", "'").replace("&#39;", "'")
                .replace("&lt;", "<").replace("&gt;", ">");
    }
}
