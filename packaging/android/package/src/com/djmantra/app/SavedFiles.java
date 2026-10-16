package com.djmantra.app;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * Every file the app saves says where it is (Marko 16.10.2026: "any file type is saved, for
 * example, playlist from beatstats or jpeg from beatstats or any other files, please post where
 * the file is, the location and name"): a long toast with the file's name and its folder, as
 * the phone's Files app shows it ("Internal storage/Documents/DJ Mantra/BeatStats/captures").
 * The same file saved again within 30 s is not announced twice (playlists, links saved as they
 * change). Logged as "Saved file: <path>" for the test tools.
 */
public final class SavedFiles {
    private static final String TAG = "DJMantra";
    private static final long QUIET_MS = 30000;
    private static final Map<String, Long> sLast = new HashMap<>();

    private SavedFiles() {
    }

    /** The folder as the Files app names it. */
    static String friendlyFolder(File file) {
        String dir = file.getParent() == null ? "" : file.getParent();
        for (String root : new String[] {"/sdcard", "/storage/emulated/0", "/storage/self/primary"}) {
            if (dir.equals(root) || dir.startsWith(root + "/")) {
                return "Internal storage" + dir.substring(root.length());
            }
        }
        return dir;
    }

    public static void announce(Context context, String path) {
        if (path == null || path.isEmpty()) {
            return;
        }
        final File file = new File(path);
        Log.i(TAG, "Saved file: " + file.getAbsolutePath());
        final long now = System.currentTimeMillis();
        synchronized (sLast) {
            Long last = sLast.get(file.getAbsolutePath());
            if (last != null && now - last < QUIET_MS) {
                return;
            }
            sLast.put(file.getAbsolutePath(), now);
        }
        if (context == null) {
            return;
        }
        final Context app = context.getApplicationContext() != null
                ? context.getApplicationContext() : context;
        final String text = "Saved " + file.getName() + "\nin " + friendlyFolder(file);
        new Handler(Looper.getMainLooper()).post(
                () -> Toast.makeText(app, text, Toast.LENGTH_LONG).show());
    }
}
