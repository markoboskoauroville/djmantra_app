// DJ Mantra: the app window over the whole screen (immersive): the status
// and navigation bars hide and come back with a swipe from the edge, so they
// do not cover the decks (round 2 on the phones: the navigation bar sat over
// deck 2 in landscape and over the crossfader in portrait).
package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

public final class WindowBridge {
    private WindowBridge() {}

    private static boolean sWatching = false;

    public static void immersive(Context context) {
        if (!(context instanceof Activity)) {
            return;
        }
        final Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            Window window = activity.getWindow();
            if (Build.VERSION.SDK_INT >= 30) {
                window.setDecorFitsSystemWindows(false);
                WindowInsetsController controller = window.getInsetsController();
                if (controller != null) {
                    controller.hide(WindowInsets.Type.systemBars());
                    controller.setSystemBarsBehavior(
                            WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                }
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

    // The system shows the bars again after a rotation (round 6: status and
    // navigation bars back over the bottom row in portrait). Hide them again
    // whenever a layout pass finds them shown for good; bars pulled in with
    // a swipe are transient and do not count as visible.
    private static void watch(final Window window) {
        if (sWatching) {
            return;
        }
        sWatching = true;
        final View decor = window.getDecorView();
        // Also every 1.5 s: showing the bars doesn't always lay the window
        // out again (round 7: bars over the song picker)
        decor.postDelayed(new Runnable() {
            @Override
            public void run() {
                hideIfShown(window, decor);
                decor.postDelayed(this, 1500);
            }
        }, 1500);
        decor.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        hideIfShown(window, decor);
                    }
                });
    }

    private static void hideIfShown(Window window, View decor) {
        WindowInsets insets = decor.getRootWindowInsets();
        WindowInsetsController controller = window.getInsetsController();
        if (insets != null && controller != null
                && insets.isVisible(WindowInsets.Type.systemBars())) {
            controller.hide(WindowInsets.Type.systemBars());
        }
    }
}
