// DJ Mantra: the app window and the system bars.
//
// The status and navigation bars are hidden while you play. A swipe from the
// edge brings them back, and then the app makes room for them instead of
// lying under them (owner, 10.10.2026: "the user interface is moving
// according to the navigation buttons coming or going"): insets() tells the
// app how much of each edge the bars take, and the app shrinks its content by
// that much. After a few seconds without the bars being needed they hide
// again and the app takes the whole screen back.
package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.graphics.Insets;
import android.graphics.Rect;
import android.os.Build;
import android.os.SystemClock;
import android.view.DisplayCutout;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

public final class WindowBridge {
    private WindowBridge() {}

    // How long the bars stay after they came back (a swipe, a rotation)
    private static final long SHOW_MS = 4000;

    private static boolean sWatching = false;
    private static long sShownSince = 0;
    // left, top, right, bottom in pixels: read from the Qt thread
    private static volatile int[] sInsets = new int[] {0, 0, 0, 0};

    public static void immersive(Context context) {
        if (!(context instanceof Activity)) {
            return;
        }
        final Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            Window window = activity.getWindow();
            if (Build.VERSION.SDK_INT >= 30) {
                // The app draws edge to edge (Android 15 does so anyway) and
                // keeps out of the bars itself, with insets()
                window.setDecorFitsSystemWindows(false);
                WindowInsetsController controller = window.getInsetsController();
                if (controller != null) {
                    controller.hide(WindowInsets.Type.systemBars());
                    // A swipe shows the bars for real (not as an overlay), so
                    // the app sees them and makes room
                    controller.setSystemBarsBehavior(
                            WindowInsetsController.BEHAVIOR_DEFAULT);
                }
                sShownSince = 0;
                watch(window);
            } else {
                window.getDecorView().setSystemUiVisibility(
                        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN);
            }
        });
    }

    /// The room the visible system bars and the camera hole take: left, top,
    /// right, bottom (px). Bars: 0 while they are hidden.
    public static int[] insets() {
        return sInsets;
    }

    // Every 300 ms: note the bars' room for the app, and hide the bars again
    // once they have been there for SHOW_MS
    private static void watch(final Window window) {
        if (sWatching) {
            return;
        }
        sWatching = true;
        final View decor = window.getDecorView();
        decor.postDelayed(new Runnable() {
            @Override
            public void run() {
                update(window, decor);
                decor.postDelayed(this, 300);
            }
        }, 300);
    }

    private static void update(Window window, View decor) {
        WindowInsets insets = decor.getRootWindowInsets();
        if (insets == null) {
            return;
        }
        // The bars' full room while they are visible, nothing while hidden:
        // not the animated value (round 10: the margins went to 0 while the
        // bar was still on screen)
        final boolean visible = insets.isVisible(WindowInsets.Type.systemBars());
        Insets bars = visible
                ? insets.getInsetsIgnoringVisibility(WindowInsets.Type.systemBars())
                : Insets.NONE;
        // The camera hole (display cutout): nothing may sit under it (Marko,
        // 12.10.2026: "spread the user interface around this hole"). A hole on
        // a side (landscape) or off centre at the top: the app keeps out of
        // that edge's band. A hole in the middle of the top edge (the Nothing
        // phones, most phones): no band, the skin's top row keeps its middle
        // free (logo left, key badge right).
        Insets cutout = insets.getInsetsIgnoringVisibility(WindowInsets.Type.displayCutout());
        int cutTop = cutout.top;
        DisplayCutout shape = insets.getDisplayCutout();
        if (shape != null && cutTop > 0) {
            Rect hole = shape.getBoundingRectTop();
            int width = decor.getWidth();
            if (!hole.isEmpty() && width > 0 && hole.left >= width * 3 / 8
                    && hole.right <= width * 5 / 8) {
                cutTop = 0;
            }
        }
        sInsets = new int[] {Math.max(bars.left, cutout.left), Math.max(bars.top, cutTop),
                Math.max(bars.right, cutout.right), Math.max(bars.bottom, cutout.bottom)};
        if (!visible) {
            sShownSince = 0;
            return;
        }
        long now = SystemClock.uptimeMillis();
        if (sShownSince == 0) {
            sShownSince = now;
        } else if (now - sShownSince > SHOW_MS) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
            }
            sShownSince = 0;
        }
    }
}
