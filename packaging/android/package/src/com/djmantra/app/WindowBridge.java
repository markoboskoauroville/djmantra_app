// DJ Mantra: the app window over the whole screen (immersive): the status
// and navigation bars hide and come back with a swipe from the edge, so they
// do not cover the decks (round 2 on the phones: the navigation bar sat over
// deck 2 in landscape and over the crossfader in portrait).
package com.djmantra.app;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

public final class WindowBridge {
    private WindowBridge() {}

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
}
