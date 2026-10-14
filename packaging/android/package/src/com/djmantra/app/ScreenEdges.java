package com.djmantra.app;

import android.app.Activity;
import android.os.Build;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

/**
 * The app's own screens opened as their own Android activities (BEATSTATS, the Suno login) as
 * the main screen (Marko 14.10.2026: "that part is not adapting to the user interface of Android
 * as the other parts are doing ... it's cropped"): the system bars hidden (a swipe from the edge
 * shows them for a moment), the content kept clear of the camera cutout and of the bars while
 * they show. Android 15+ draws every app edge to edge, so an activity that does nothing about
 * insets runs under the status and navigation bars. Each such screen also has its own BACK
 * button, as Android's back is hidden with the bars.
 */
public final class ScreenEdges {
    private ScreenEdges() {
    }

    public static void fit(Activity activity, View root) {
        Window window = activity.getWindow();
        if (Build.VERSION.SDK_INT >= 30) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
            root.setOnApplyWindowInsetsListener((view, insets) -> {
                android.graphics.Insets edge = insets.getInsets(
                        WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout());
                view.setPadding(edge.left, edge.top, edge.right, edge.bottom);
                return WindowInsets.CONSUMED;
            });
        } else {
            root.setFitsSystemWindows(true);
        }
    }

    /** Again when the screen comes back (Android shows the bars after another app). */
    public static void hideAgain(Activity activity) {
        if (Build.VERSION.SDK_INT >= 30) {
            WindowInsetsController controller = activity.getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
            }
        }
    }
}
