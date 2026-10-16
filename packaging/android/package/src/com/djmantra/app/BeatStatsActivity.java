package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.PixelCopy;
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
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * BEATSTATS (Marko 14.10.2026): beatstats.com in a real browser engine (Cloudflare's check is
 * passed by hand once, the cookie stays), the categories flipped there as on the site. READ CHART
 * reads the chart's rows from the page itself (exact text, no AI, no OCR: Marko 14.10.2026):
 * {chart_title, entries: [{rank, title, artist, beatport (the Beatport track ID)}]}, stored in
 * djmantra_app/BeatStats/charts.json (newest first, one per chart title) for the app's chart view.
 * V81 (Marko 16.10.2026: READ CHART "right now doesn't find anything. We need to find out why"):
 * besides the chart rows, any track links on the page are read; a page with none is saved as
 * BeatStats/pages/page_<time>.html with what was on it, to see why. CAPTURE PAGE (Marko: "screen
 * capture, page down, screen capture, page down until come to the end of the page and then connect
 * everything in one JPEG file ... from top to the bottom"): BeatStats/captures/<title>_<time>.jpg.
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
        Button back = new Button(this);
        back.setText("BACK");
        back.setOnClickListener(v -> onBackPressed());
        Button read = new Button(this);
        read.setText("READ CHART");
        read.setOnClickListener(v -> readChart());
        Button capture = new Button(this);
        capture.setText("CAPTURE PAGE");
        capture.setOnClickListener(v -> capturePage());
        Button close = new Button(this);
        close.setText("CLOSE");
        close.setOnClickListener(v -> finish());
        mStatus = new TextView(this);
        mStatus.setTextColor(Color.parseColor("#EEF0FF"));
        mStatus.setPadding(16, 0, 16, 0);
        mStatus.setText("Open a chart, then READ CHART");
        bar.addView(back);
        bar.addView(read);
        bar.addView(capture);
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
        ScreenEdges.fit(this, root);
        mWeb.loadUrl(getSharedPreferences("djmantra_beatstats", MODE_PRIVATE).getString("url", HOME));
    }

    @Override
    protected void onResume() {
        super.onResume();
        ScreenEdges.hideAgain(this);
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
    // the Beatport track ID from the row's Beatport link (the exact record; its ISRC later).
    // V81: no such rows (another kind of page, another layout): every link to a track (BeatStats'
    // own /track/<name>/<id>, its ID is Beatport's, or beatport.com/track/...), in page order,
    // with its row's title and artists; same-origin frames too. `page` says what was there.
    private static final String READ_JS = "(function(){"
            + "var docs=[document];[].slice.call(document.querySelectorAll('iframe')).forEach(function(f){"
            + "try{if(f.contentDocument)docs.push(f.contentDocument);}catch(e){}});"
            + "var head=document.querySelector('.chart-table-head');"
            + "var t=head?((head.querySelector('small')||{}).innerText||'')+' '+((head.querySelector('h2')||{}).innerText||''):document.title;"
            + "var rows=[];var all=function(sel){var r=[];docs.forEach(function(d){r=r.concat([].slice.call(d.querySelectorAll(sel)));});return r;};"
            + "all('article.rank-row--track').forEach(function(r){"
            + "var a=r.querySelector('a[href*=\"beatport.com/track/\"]');"
            + "var m=a?a.href.match(/\\/(\\d+)(\\?|$)/):null;"
            + "if(!m){var own=r.querySelector('a[href*=\"/track/\"]')||r;var u=own.getAttribute('href')||r.getAttribute('data-row-url')||'';m=u.match(/\\/track\\/[^\\/]+\\/(\\d+)/);}"
            + "var artists=[].slice.call(r.querySelectorAll('.artist-list > span')).map(function(s){return s.innerText.trim();}).filter(function(x){return x;});"
            + "rows.push({rank:parseInt((r.querySelector('.rank-number')||{}).innerText)||rows.length+1,"
            + "title:((r.querySelector('.rank-name strong')||{}).innerText||'').trim(),"
            + "artist:artists.join(', '),beatport:m?m[1]:'',url:a?a.href:''});});"
            + "var how='chart rows';"
            + "if(!rows.length){how='track links';var seen={};"
            + "all('a[href*=\"/track/\"]').forEach(function(a){"
            + "var m=(a.href||'').match(/\\/track\\/[^\\/?#]+\\/(\\d+)/);if(!m||seen[m[1]])return;seen[m[1]]=1;"
            + "var up=a.parentElement||a;var box=up.closest('article,li,tr,[class*=row],[class*=item],[class*=card]')||up;"
            + "var name=box.querySelector('strong')||box.querySelector('h3,h4')||box.querySelector('[class*=title],[class*=name]');"
            + "var title=((name&&name.innerText)||(a.getAttribute('aria-label')||'').replace(/^Open /,'')||a.innerText||'').trim().split('\\n')[0];"
            + "var art=box.querySelector('.artist-list,[class*=artist]');"
            + "var rank=box.querySelector('[class*=rank-number],[class*=position],[class*=rank]');"
            + "rows.push({rank:(rank&&parseInt(rank.innerText))||rows.length+1,title:title,"
            + "artist:art?art.innerText.trim().replace(/\\s*\\n\\s*/g,', '):'',beatport:m[1],url:a.href});});}"
            + "var page={url:location.href,title:document.title,articles:all('article').length,"
            + "trackRows:all('article.rank-row--track').length,trackLinks:all('a[href*=\"/track/\"]').length,"
            + "frames:document.querySelectorAll('iframe').length,text:(document.body?document.body.innerText.length:0)};"
            + "return JSON.stringify({chart_title:t.trim(),url:location.href,how:how,page:page,entries:rows});})()";

    private void readChart() {
        mStatus.setText("Reading the chart ...");
        mWeb.evaluateJavascript(READ_JS, value -> {
            try {
                String json = new JSONArray("[" + value + "]").getString(0);
                JSONObject chart = new JSONObject(json);
                JSONObject page = chart.optJSONObject("page");
                chart.remove("page");
                int count = chart.getJSONArray("entries").length();
                Log.i(TAG, "Read " + chart.optString("url") + ": " + count + " (" + chart.optString("how")
                        + "), page " + page);
                if (count == 0) {
                    // V81: why nothing? the page is kept to look at
                    String saved = savePage();
                    mStatus.setText("No songs on this page (" + (page == null ? "?"
                            : page.optInt("trackRows") + " chart rows, " + page.optInt("trackLinks")
                                    + " track links, " + page.optInt("articles") + " rows")
                            + "). Saved: " + saved + ". Open a chart's TRACKS list.");
                    return;
                }
                chart.put("read", System.currentTimeMillis());
                store(chart);
                Log.i(TAG, "Chart " + chart.optString("chart_title") + ": " + count + " songs");
                mStatus.setText(chart.optString("chart_title") + ": " + count + " songs read ("
                        + chart.optString("how") + "), finding ISRCs ...");
                findCodes(chart);
            } catch (Exception e) {
                Log.w(TAG, "read: " + e);
                mStatus.setText("Could not read this page: " + e.getMessage());
            }
        });
    }

    /** The page's HTML as the browser has it now, into BeatStats/pages/; returns the file name. */
    private String savePage() {
        final String name = "page_" + stamp() + ".html";
        mWeb.evaluateJavascript("(function(){return '<!-- '+location.href+' -->\\n'+document.documentElement.outerHTML;})()",
                value -> new Thread(() -> {
                    try {
                        String html = new JSONArray("[" + value + "]").getString(0);
                        File dir = new File(DIR, "pages");
                        dir.mkdirs();
                        try (FileOutputStream out = new FileOutputStream(new File(dir, name))) {
                            out.write(html.getBytes(StandardCharsets.UTF_8));
                        }
                        Log.i(TAG, "page saved: " + name + " (" + html.length() + " chars)");
                    } catch (Exception e) {
                        Log.w(TAG, "save page: " + e);
                    }
                }).start());
        return "pages/" + name;
    }

    private static String stamp() {
        return new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(new Date());
    }

    // --- CAPTURE PAGE ---------------------------------------------------------------------------
    // The page from top to bottom as ONE JPEG (Marko 16.10.2026): scroll to the top, capture what
    // the browser shows, page down, capture, ... to the end; each capture adds only what is new
    // (a header stuck to the top is cut from all but the first); then joined top to bottom. Pages
    // that scroll an inner box instead of the window: that box is scrolled.
    private static final String SCROLL_JS = "(function(){"
            + "if(!window.__djm){var s=document.scrollingElement||document.documentElement;var el=null;"
            + "if(s.scrollHeight<=window.innerHeight+40){var best=0;[].slice.call(document.querySelectorAll('*')).forEach(function(e){"
            + "var o=getComputedStyle(e).overflowY;if((o=='auto'||o=='scroll')&&e.scrollHeight>e.clientHeight+40&&e.clientHeight>window.innerHeight*0.5&&e.scrollHeight>best){best=e.scrollHeight;el=e;}});}"
            + "var top=0;[].slice.call(document.querySelectorAll('body *')).forEach(function(e){var p=getComputedStyle(e).position;"
            + "if(p=='fixed'||p=='sticky'){var r=e.getBoundingClientRect();if(r.top<=2&&r.bottom>0&&r.bottom<window.innerHeight*0.35&&r.width>window.innerWidth*0.5)top=Math.max(top,r.bottom);}});"
            + "window.__djm={el:el,top:top};}"
            + "var d=window.__djm;return d;})";
    private static final int MAX_HEIGHT = 40000; // px of the JPEG (memory; JPEG's limit is 65535)
    private static final int MAX_SCREENS = 120;

    private final List<Bitmap> mStrips = new ArrayList<>();
    private boolean mCapturing = false;
    private double mCovered = 0;   // CSS px of the page captured so far
    private double mHeader = 0;    // CSS px of a header stuck to the top
    private int mScreens = 0;
    private String mCaptureTitle = "";

    private void capturePage() {
        if (mCapturing) {
            return;
        }
        mCapturing = true;
        mStrips.clear();
        mCovered = 0;
        mScreens = 0;
        status("Capturing the page ...");
        mWeb.evaluateJavascript("(function(){window.__djm=null;var d=" + SCROLL_JS + "();"
                + "if(d.el)d.el.scrollTop=0;else window.scrollTo(0,0);return JSON.stringify({top:d.top,title:document.title});})()",
                value -> {
                    try {
                        JSONObject info = new JSONObject(new JSONArray("[" + value + "]").getString(0));
                        mHeader = info.optDouble("top", 0);
                        mCaptureTitle = info.optString("title", "beatstats");
                    } catch (Exception e) {
                        mHeader = 0;
                    }
                    mMain.postDelayed(this::captureStep, 900);
                });
    }

    // where the page is now, then capture the part not yet captured
    private void captureStep() {
        mWeb.evaluateJavascript("(function(){var d=window.__djm||{};var e=d.el;"
                + "return JSON.stringify({y:e?e.scrollTop:window.scrollY,vh:e?e.clientHeight:window.innerHeight,"
                + "h:e?e.scrollHeight:(document.scrollingElement||document.documentElement).scrollHeight});})()",
                value -> {
                    try {
                        JSONObject at = new JSONObject(new JSONArray("[" + value + "]").getString(0));
                        grab(at.getDouble("y"), at.getDouble("vh"), at.getDouble("h"));
                    } catch (Exception e) {
                        failCapture("position: " + e.getMessage());
                    }
                });
    }

    private void grab(double y, double vh, double h) {
        final Bitmap shot = Bitmap.createBitmap(mWeb.getWidth(), mWeb.getHeight(), Bitmap.Config.ARGB_8888);
        int[] where = new int[2];
        mWeb.getLocationInWindow(where);
        Rect rect = new Rect(where[0], where[1], where[0] + mWeb.getWidth(), where[1] + mWeb.getHeight());
        PixelCopy.request(getWindow(), rect, shot, result -> {
            if (result != PixelCopy.SUCCESS) {
                shot.recycle();
                failCapture("screen copy " + result);
                return;
            }
            // view px per CSS px; the new part runs from what is covered (below the stuck header)
            double ratio = mWeb.getHeight() / Math.max(1.0, vh);
            double from = mScreens == 0 ? y : Math.max(mCovered, y + mHeader);
            int top = (int) Math.round((from - y) * ratio);
            int bottom = Math.min(shot.getHeight(), (int) Math.round(vh * ratio));
            if (bottom - top > 2) {
                Bitmap strip = Bitmap.createBitmap(shot, 0, top, shot.getWidth(), bottom - top)
                        .copy(Bitmap.Config.RGB_565, false);
                mStrips.add(strip);
            }
            shot.recycle();
            mCovered = y + vh;
            ++mScreens;
            int height = 0;
            for (Bitmap b : mStrips) {
                height += b.getHeight();
            }
            status("Capturing the page: screen " + mScreens + " (" + (int) Math.min(100, 100 * mCovered / Math.max(1, h)) + " %)");
            boolean end = mCovered >= h - 2 || mScreens >= MAX_SCREENS || height >= 3 * MAX_HEIGHT;
            if (end) {
                joinCapture();
                return;
            }
            // page down: the next screen starts where this one ends, under the stuck header
            final double next = mCovered - mHeader;
            final double before = y;
            mWeb.evaluateJavascript("(function(){var d=window.__djm||{};if(d.el)d.el.scrollTop=" + next
                    + ";else window.scrollTo(0," + next + ");return (d.el?d.el.scrollTop:window.scrollY);})()",
                    moved -> {
                        double now;
                        try {
                            now = Double.parseDouble(moved);
                        } catch (Exception e) {
                            now = next;
                        }
                        if (now <= before + 1) {
                            joinCapture(); // the page does not scroll further
                        } else {
                            // time for lazy images and the page to draw
                            mMain.postDelayed(this::captureStep, 700);
                        }
                    });
        }, mMain);
    }

    // all the strips top to bottom in one JPEG, scaled down if longer than MAX_HEIGHT
    private void joinCapture() {
        final List<Bitmap> strips = new ArrayList<>(mStrips);
        mStrips.clear();
        status("Joining " + strips.size() + " screens ...");
        new Thread(() -> {
            try {
                int width = strips.isEmpty() ? 0 : strips.get(0).getWidth();
                int height = 0;
                for (Bitmap b : strips) {
                    height += b.getHeight();
                }
                if (width == 0 || height == 0) {
                    failCapture("nothing captured");
                    return;
                }
                float scale = Math.min(1f, (float) MAX_HEIGHT / height);
                Bitmap page = Bitmap.createBitmap(Math.max(1, Math.round(width * scale)),
                        Math.max(1, Math.round(height * scale)), Bitmap.Config.RGB_565);
                Canvas canvas = new Canvas(page);
                canvas.scale(scale, scale);
                int y = 0;
                for (Bitmap b : strips) {
                    canvas.drawBitmap(b, 0, y, null);
                    y += b.getHeight();
                    b.recycle();
                }
                File dir = new File(DIR, "captures");
                dir.mkdirs();
                String name = (mCaptureTitle.isEmpty() ? "beatstats" : mCaptureTitle)
                        .replaceAll("[\\\\/:*?\"<>|]", "_").trim();
                if (name.length() > 80) {
                    name = name.substring(0, 80);
                }
                File file = new File(dir, name + "_" + stamp() + ".jpg");
                try (FileOutputStream out = new FileOutputStream(file)) {
                    page.compress(Bitmap.CompressFormat.JPEG, 88, out);
                }
                Log.i(TAG, "capture: " + file + " " + page.getWidth() + "x" + page.getHeight() + ", "
                        + strips.size() + " screens");
                status("Saved " + page.getWidth() + "x" + page.getHeight() + " JPEG (" + strips.size()
                        + " screens): BeatStats/captures/" + file.getName());
                page.recycle();
            } catch (Throwable e) {
                Log.w(TAG, "capture join: " + e);
                status("Capture failed: " + e.getMessage());
            } finally {
                mMain.post(() -> mCapturing = false);
            }
        }).start();
    }

    private void failCapture(String why) {
        Log.w(TAG, "capture: " + why);
        for (Bitmap b : mStrips) {
            b.recycle();
        }
        mStrips.clear();
        status("Capture failed: " + why);
        mMain.post(() -> mCapturing = false);
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
