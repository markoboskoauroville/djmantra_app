package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.JavascriptInterface;
import android.webkit.URLUtil;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * SUNO GET (Marko 14.10.2026): a Suno song page in a browser inside the app, logged in to Suno
 * as the user (once, the login stays). Suno gives the audio only to its logged-in owner, so the
 * song comes through Suno's own Download (MP3 / WAV): whatever the page downloads is saved as
 * djmantra_app/Suno/<title>.<ext>, the title being the song's (the app then puts it in the
 * playlist, matched by that title).
 */
public final class SunoActivity extends Activity {
    private static final String TAG = "DJMantraSuno";
    private static final String DIR = "/sdcard/Music/djmantra_app/Suno";

    private WebView mWeb;
    private TextView mStatus;
    private String mTitle = "";
    private final Handler mMain = new Handler(Looper.getMainLooper());

    /** Opens `url` (a Suno song link); `title` names the file saved. */
    public static void open(Context context, String url, String title) {
        Intent intent = new Intent(context, SunoActivity.class);
        intent.putExtra("url", url);
        intent.putExtra("title", title == null ? "" : title);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        mTitle = getIntent().getStringExtra("title");
        if (mTitle == null) {
            mTitle = "";
        }
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#0B1026"));

        LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(12, 12, 12, 12);
        Button close = new Button(this);
        close.setText("DONE");
        close.setOnClickListener(v -> finish());
        mStatus = new TextView(this);
        mStatus.setTextColor(Color.parseColor("#EEF0FF"));
        mStatus.setPadding(16, 0, 16, 0);
        mStatus.setText("Log in to Suno once, then the song's ... menu -> Download -> MP3");
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
        mWeb.setWebViewClient(new WebViewClient());
        mWeb.addJavascriptInterface(new BlobSaver(), "DJMantraSuno");
        mWeb.setDownloadListener((url, agent, disposition, mime, length) -> {
            String name = URLUtil.guessFileName(url, disposition, mime);
            String ext = name.contains(".") ? name.substring(name.lastIndexOf('.')) : ".mp3";
            if (url.startsWith("blob:")) {
                // the page made the file itself: read back through the page, in pieces
                status("Saving ...");
                mWeb.evaluateJavascript("(function(){fetch('" + url + "').then(r=>r.blob()).then(b=>{"
                        + "var f=new FileReader();f.onloadend=function(){"
                        + "DJMantraSuno.save(f.result.substring(f.result.indexOf(',')+1),'" + ext + "');};"
                        + "f.readAsDataURL(b);}).catch(e=>DJMantraSuno.fail(String(e)));})()", null);
            } else if (url.startsWith("http")) {
                status("Saving ...");
                final String cookies = CookieManager.getInstance().getCookie(url);
                new Thread(() -> download(url, cookies, agent, ext)).start();
            }
        });
        root.addView(mWeb, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        setContentView(root);
        mWeb.loadUrl(getIntent().getStringExtra("url"));
    }

    @Override
    public void onBackPressed() {
        if (mWeb != null && mWeb.canGoBack()) {
            mWeb.goBack();
        } else {
            super.onBackPressed();
        }
    }

    private File target(String ext) {
        File dir = new File(DIR);
        dir.mkdirs();
        String name = mTitle.isEmpty() ? "Suno " + System.currentTimeMillis() : mTitle;
        return new File(dir, name.replaceAll("[\\\\/:*?\"<>|]", "_") + ext);
    }

    private void download(String url, String cookies, String agent, String ext) {
        try {
            HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
            c.setInstanceFollowRedirects(true);
            if (cookies != null) {
                c.setRequestProperty("Cookie", cookies);
            }
            c.setRequestProperty("User-Agent", agent);
            File file = target(ext);
            try (InputStream in = c.getInputStream(); FileOutputStream out = new FileOutputStream(file)) {
                byte[] buffer = new byte[65536];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            }
            saved(file);
        } catch (Exception e) {
            Log.w(TAG, "download: " + e);
            status("Could not save: " + e.getMessage());
        }
    }

    private void saved(File file) {
        Log.i(TAG, "saved " + file + " (" + file.length() + " bytes)");
        status("Saved: " + file.getName() + " (" + file.length() / 1024 + " KB). DONE to go back.");
    }

    private void status(String text) {
        mMain.post(() -> mStatus.setText(text));
    }

    private final class BlobSaver {
        @JavascriptInterface
        public void save(String base64, String ext) {
            try {
                File file = target(ext);
                try (FileOutputStream out = new FileOutputStream(file)) {
                    out.write(Base64.decode(base64, Base64.DEFAULT));
                }
                saved(file);
            } catch (Exception e) {
                Log.w(TAG, "save: " + e);
                status("Could not save: " + e.getMessage());
            }
        }

        @JavascriptInterface
        public void fail(String error) {
            status("Could not save: " + error);
        }
    }
}
