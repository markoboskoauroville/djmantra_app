package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.io.ByteArrayOutputStream;

/**
 * BEATSTATS (Marko 14.10.2026): beatstats.com in a real browser engine (Cloudflare's check is
 * passed by hand once, the cookie stays), the categories flipped there as on the site. READ CHART
 * reads the chart's rows from the page itself (exact text, no AI, no OCR: Marko 14.10.2026):
 * {chart_title, entries: [{rank, title, artist, beatport (the Beatport track ID)}]}, stored in
 * djmantra_app/BeatStats/charts.json (newest first, one per chart title) for the app's chart view.
 */
public final class BeatStatsActivity extends Activity {
    private static final String TAG = "DJMantraBeatStats";
    private static final String HOME = "https://www.beatstats.com/";
    private static final String DIR = "/sdcard/Music/djmantra_app/BeatStats";

    private WebView mWeb;
    private TextView mStatus;
    private final Handler mMain = new Handler(Looper.getMainLooper());

    /** Opens the BeatStats browser (from QmlDJMantra::openBeatStats). */
    public static void open(Context context) {
        Intent intent = new Intent(context, BeatStatsActivity.class);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#0B1026"));

        LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(12, 12, 12, 12);
        Button read = new Button(this);
        read.setText("READ CHART");
        read.setOnClickListener(v -> readChart());
        Button close = new Button(this);
        close.setText("CLOSE");
        close.setOnClickListener(v -> finish());
        mStatus = new TextView(this);
        mStatus.setTextColor(Color.parseColor("#EEF0FF"));
        mStatus.setPadding(16, 0, 16, 0);
        mStatus.setText("Open a chart, then READ CHART");
        bar.addView(read);
        bar.addView(close);
        bar.addView(mStatus, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        root.addView(bar, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        mWeb = new WebView(this);
        WebSettings settings = mWeb.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        CookieManager.getInstance().setAcceptCookie(true);
        CookieManager.getInstance().setAcceptThirdPartyCookies(mWeb, true);
        mWeb.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                getSharedPreferences("djmantra_beatstats", MODE_PRIVATE).edit().putString("url", url).apply();
            }
        });
        root.addView(mWeb, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        setContentView(root);
        mWeb.loadUrl(getSharedPreferences("djmantra_beatstats", MODE_PRIVATE).getString("url", HOME));
    }

    @Override
    public void onBackPressed() {
        if (mWeb != null && mWeb.canGoBack()) {
            mWeb.goBack();
        } else {
            super.onBackPressed();
        }
    }

    // the chart rows as BeatStats draws them (article.rank-row--track): rank, title, artists and
    // the Beatport track ID from the row's Beatport link (the exact record; its ISRC later)
    private static final String READ_JS = "(function(){"
            + "var head=document.querySelector('.chart-table-head');"
            + "var t=head?((head.querySelector('small')||{}).innerText||'')+' '+((head.querySelector('h2')||{}).innerText||''):document.title;"
            + "var rows=[].slice.call(document.querySelectorAll('article.rank-row--track')).map(function(r){"
            + "var a=r.querySelector('a[href*=\"beatport.com/track/\"]');"
            + "var m=a?a.href.match(/\\/(\\d+)(\\?|$)/):null;"
            + "var artists=[].slice.call(r.querySelectorAll('.artist-list > span')).map(function(s){return s.innerText.trim();}).filter(function(x){return x;});"
            + "return {rank:parseInt((r.querySelector('.rank-number')||{}).innerText)||0,"
            + "title:((r.querySelector('.rank-name strong')||{}).innerText||'').trim(),"
            + "artist:artists.join(', '),beatport:m?m[1]:'',url:a?a.href:''};});"
            + "return JSON.stringify({chart_title:t.trim(),url:location.href,entries:rows});})()";

    private void readChart() {
        mStatus.setText("Reading the chart ...");
        mWeb.evaluateJavascript(READ_JS, value -> {
            try {
                String json = new JSONArray("[" + value + "]").getString(0);
                JSONObject chart = new JSONObject(json);
                int count = chart.getJSONArray("entries").length();
                if (count == 0) {
                    mStatus.setText("No chart rows on this page: open a chart (TRACKS, a genre)");
                    return;
                }
                chart.put("read", System.currentTimeMillis());
                store(chart);
                Log.i(TAG, "Chart " + chart.optString("chart_title") + ": " + count + " songs");
                mStatus.setText(chart.optString("chart_title") + ": " + count + " songs read, finding ISRCs ...");
                findCodes(chart);
            } catch (Exception e) {
                Log.w(TAG, "read: " + e);
                mStatus.setText("Could not read this page: " + e.getMessage());
            }
        });
    }

    /**
     * Off the UI thread: each song's ISRC (from Beatport) and, when logged in to TIDAL, its exact
     * TIDAL track ID (by ISRC), stored with the chart as they come.
     */
    private void findCodes(JSONObject chart) {
        final Context app = getApplicationContext();
        new Thread(() -> {
            try {
                JSONArray entries = chart.getJSONArray("entries");
                int codes = 0;
                for (int i = 0; i < entries.length(); ++i) {
                    JSONObject song = entries.getJSONObject(i);
                    String code = isrc(song.optString("beatport"));
                    if (code.isEmpty()) {
                        continue;
                    }
                    ++codes;
                    song.put("isrc", code);
                    song.put("tidal", TidalBridge.trackByIsrc(app, code));
                    status(chart.optString("chart_title") + ": ISRC " + codes + "/" + entries.length());
                }
                store(chart);
                status(chart.optString("chart_title") + ": " + entries.length() + " songs, " + codes + " ISRCs");
            } catch (Exception e) {
                Log.w(TAG, "codes: " + e);
                status("ISRCs: " + e.getMessage());
            }
        }).start();
    }

    /**
     * The ISRC of Beatport track `id` (the exact recording's code, Marko 14.10.2026): from the
     * track page's embedded data ("isrc":"..."), "" if not found. Blocking: off the UI thread.
     */
    public static String isrc(String id) {
        if (id == null || id.isEmpty()) {
            return "";
        }
        try {
            HttpURLConnection c = (HttpURLConnection) new URL("https://www.beatport.com/track/x/" + id).openConnection();
            c.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 16; Pixel 7) AppleWebKit/537.36 "
                    + "(KHTML, like Gecko) Chrome/140.0 Mobile Safari/537.36");
            c.setConnectTimeout(15000);
            c.setReadTimeout(20000);
            int code = c.getResponseCode();
            if (code >= 400) {
                Log.w(TAG, "Beatport " + id + ": HTTP " + code);
                return "";
            }
            InputStream in = c.getInputStream();
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            byte[] buffer = new byte[16384];
            int n;
            while ((n = in.read(buffer)) > 0) {
                bytes.write(buffer, 0, n);
            }
            java.util.regex.Matcher m = java.util.regex.Pattern.compile("\"isrc\"\\s*:\\s*\"([A-Z0-9]{12})\"")
                    .matcher(bytes.toString("UTF-8"));
            String isrc = m.find() ? m.group(1) : "";
            Log.i(TAG, "Beatport " + id + " ISRC " + isrc);
            return isrc;
        } catch (Exception e) {
            Log.w(TAG, "isrc " + id + ": " + e);
            return "";
        }
    }

    /** charts.json: newest first, one chart per title. */
    private static synchronized void store(JSONObject chart) throws Exception {
        File dir = new File(DIR);
        dir.mkdirs();
        File file = new File(dir, "charts.json");
        JSONArray old = new JSONArray();
        if (file.exists()) {
            byte[] data = java.nio.file.Files.readAllBytes(file.toPath());
            old = new JSONArray(new String(data, StandardCharsets.UTF_8));
        }
        JSONArray charts = new JSONArray();
        charts.put(chart);
        for (int i = 0; i < old.length(); ++i) {
            JSONObject other = old.getJSONObject(i);
            if (!other.optString("chart_title").equalsIgnoreCase(chart.optString("chart_title"))) {
                charts.put(other);
            }
        }
        try (FileOutputStream out = new FileOutputStream(file)) {
            out.write(charts.toString().getBytes(StandardCharsets.UTF_8));
        }
    }

    private void status(String text) {
        mMain.post(() -> mStatus.setText(text));
    }
}
