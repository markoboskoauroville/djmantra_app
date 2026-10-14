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
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.io.ByteArrayOutputStream;

/**
 * BEATSTATS (Marko 14.10.2026): beatstats.com in a real browser engine (Cloudflare's check is
 * passed by hand once, the cookie stays), the categories flipped there as on the site. READ CHART
 * takes the page's text and Groq (a text model, the key typed once in Settings on the phone)
 * turns it into {chart_title, entries: [{rank, artist, title}]}, stored in
 * djmantra_app/BeatStats/charts.json (newest first, one per chart title) for the app's chart view.
 */
public final class BeatStatsActivity extends Activity {
    private static final String TAG = "DJMantraBeatStats";
    private static final String HOME = "https://www.beatstats.com/";
    private static final String DIR = "/sdcard/Music/djmantra_app/BeatStats";

    private WebView mWeb;
    private TextView mStatus;
    private String mKey = "";
    private final Handler mMain = new Handler(Looper.getMainLooper());

    /** Opens the BeatStats browser (from QmlDJMantra::openBeatStats). */
    public static void open(Context context, String groqKey) {
        Intent intent = new Intent(context, BeatStatsActivity.class);
        intent.putExtra("key", groqKey == null ? "" : groqKey);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        mKey = getIntent().getStringExtra("key");
        if (mKey == null) {
            mKey = "";
        }
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

    private void readChart() {
        if (mKey.isEmpty()) {
            mStatus.setText("Type the Groq API key in Settings first");
            return;
        }
        mStatus.setText("Reading the chart ...");
        mWeb.evaluateJavascript("(function(){return document.title + '\\n' + document.body.innerText;})()", value -> {
            String text;
            try {
                text = new JSONArray("[" + value + "]").getString(0);
            } catch (Exception e) {
                text = value;
            }
            final String page = text.length() > 60000 ? text.substring(0, 60000) : text;
            new Thread(() -> parse(page)).start();
        });
    }

    private void parse(String page) {
        try {
            JSONObject body = new JSONObject();
            body.put("model", "llama-3.3-70b-versatile");
            body.put("temperature", 0);
            body.put("response_format", new JSONObject().put("type", "json_object"));
            JSONArray messages = new JSONArray();
            messages.put(new JSONObject().put("role", "system").put("content",
                    "You read the text of a beatstats.com chart page. Return JSON: {\"chart_title\": "
                    + "string, \"entries\": [{\"rank\": int, \"artist\": string, \"title\": string, "
                    + "\"label\": string}]}, every song of the chart in its order, the title with its "
                    + "mix / remix name. Nothing else."));
            messages.put(new JSONObject().put("role", "user").put("content", page));
            body.put("messages", messages);

            HttpURLConnection connection =
                    (HttpURLConnection) new URL("https://api.groq.com/openai/v1/chat/completions").openConnection();
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Authorization", "Bearer " + mKey);
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setConnectTimeout(20000);
            connection.setReadTimeout(120000);
            connection.setDoOutput(true);
            try (OutputStream out = connection.getOutputStream()) {
                out.write(body.toString().getBytes(StandardCharsets.UTF_8));
            }
            int code = connection.getResponseCode();
            InputStream in = code < 400 ? connection.getInputStream() : connection.getErrorStream();
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            byte[] buffer = new byte[8192];
            int n;
            while (in != null && (n = in.read(buffer)) > 0) {
                bytes.write(buffer, 0, n);
            }
            String response = bytes.toString("UTF-8");
            if (code >= 400) {
                Log.w(TAG, "Groq " + code + ": " + response);
                status("Groq error " + code);
                return;
            }
            String content = new JSONObject(response).getJSONArray("choices").getJSONObject(0)
                                     .getJSONObject("message").getString("content");
            JSONObject chart = new JSONObject(content);
            chart.put("read", System.currentTimeMillis());
            int count = chart.optJSONArray("entries") != null ? chart.getJSONArray("entries").length() : 0;
            store(chart);
            Log.i(TAG, "Chart " + chart.optString("chart_title") + ": " + count + " songs");
            status(chart.optString("chart_title") + ": " + count + " songs read");
        } catch (Exception e) {
            Log.w(TAG, "read: " + e);
            status("Could not read: " + e.getMessage());
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
