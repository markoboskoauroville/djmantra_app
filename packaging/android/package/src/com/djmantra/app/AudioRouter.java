// DJ Mantra: the phone's audio outputs, for the audio router (Marko,
// 12.10.2026: "list every output now connected ... MASTER and HEADPHONES,
// each to any of those outputs").
//
// Each output has a stable key (its type and product name, e.g.
// "usb:DJControl Mix Ultra") and Android's current device id, which changes
// when it is plugged in again. A version number goes up whenever an output
// comes or goes, so the app can refresh its list.
package com.djmantra.app;

import android.content.Context;
import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import java.util.ArrayList;

public final class AudioRouter {
    private AudioRouter() {}

    private static final String TAG = "DJMantra";
    private static volatile int sVersion = 0;
    private static boolean sWatching = false;

    private static String typeKey(int type) {
        switch (type) {
            case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER: return "speaker";
            case AudioDeviceInfo.TYPE_WIRED_HEADSET:
            case AudioDeviceInfo.TYPE_WIRED_HEADPHONES: return "wired";
            case AudioDeviceInfo.TYPE_USB_DEVICE:
            case AudioDeviceInfo.TYPE_USB_HEADSET:
            case AudioDeviceInfo.TYPE_USB_ACCESSORY: return "usb";
            case AudioDeviceInfo.TYPE_BLUETOOTH_A2DP: return "bt";
            case AudioDeviceInfo.TYPE_HDMI: return "hdmi";
            case AudioDeviceInfo.TYPE_LINE_ANALOG:
            case AudioDeviceInfo.TYPE_LINE_DIGITAL:
            case AudioDeviceInfo.TYPE_AUX_LINE: return "line";
            case AudioDeviceInfo.TYPE_DOCK: return "dock";
            default:
                // Bluetooth LE audio (API 31+): 26 headset, 27 speaker
                if (type == 26 || type == 27) {
                    return "bt";
                }
                return null; // earpiece, telephony, bus ...: not for a DJ
        }
    }

    private static String typeLabel(String key) {
        switch (key) {
            case "speaker": return "Phone speaker";
            case "wired": return "Wired headphones";
            case "usb": return "USB";
            case "bt": return "Bluetooth";
            case "hdmi": return "HDMI";
            case "line": return "Line out";
            default: return "Dock";
        }
    }

    private static AudioDeviceInfo[] devices(Context context) {
        AudioManager manager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (manager == null) {
            return new AudioDeviceInfo[0];
        }
        watch(manager);
        return manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
    }

    private static String keyOf(AudioDeviceInfo device) {
        String type = typeKey(device.getType());
        if (type == null) {
            return null;
        }
        CharSequence name = device.getProductName();
        return type + ":" + (name == null ? "" : name.toString());
    }

    /// "key|label|channels" for each output a DJ can use
    public static String[] outputs(Context context) {
        ArrayList<String> list = new ArrayList<>();
        for (AudioDeviceInfo device : devices(context)) {
            String key = keyOf(device);
            if (key == null || containsKey(list, key)) {
                continue;
            }
            String type = key.substring(0, key.indexOf(':'));
            CharSequence name = device.getProductName();
            String label = typeLabel(type);
            if (!type.equals("speaker") && name != null && name.length() > 0) {
                label += ": " + name;
            }
            int channels = 2;
            for (int count : device.getChannelCounts()) {
                channels = Math.max(channels, count);
            }
            list.add(key + "|" + label + "|" + channels);
        }
        return list.toArray(new String[0]);
    }

    private static boolean containsKey(ArrayList<String> list, String key) {
        for (String entry : list) {
            if (entry.startsWith(key + "|")) {
                return true;
            }
        }
        return false;
    }

    /// The most channels a connected USB output offers, 0 without one, -1 when it
    /// lists none (round 37: the
    /// CQ-12T runs 16 channels; a 2-channel stream into it played at ~1/4 speed)
    public static int usbOutputChannels(Context context) {
        int most = 0;
        for (AudioDeviceInfo device : devices(context)) {
            String key = keyOf(device);
            if (key == null || !key.startsWith("usb:")) {
                continue;
            }
            int[] counts = device.getChannelCounts();
            if (counts.length == 0) {
                // round 38: the CQ-12T comes with no profiles at all (Android could
                // not read them) while its driver runs it at 16 channels
                return -1;
            }
            for (int count : counts) {
                most = Math.max(most, count);
            }
        }
        return most;
    }

    /// Android's current id of the output with this key, 0 if it is not there
    public static int idFor(Context context, String key) {
        for (AudioDeviceInfo device : devices(context)) {
            if (key.equals(keyOf(device))) {
                return device.getId();
            }
        }
        return 0;
    }

    /// Goes up whenever an output comes or goes
    public static int version() {
        return sVersion;
    }

    private static synchronized void watch(AudioManager manager) {
        if (sWatching) {
            return;
        }
        sWatching = true;
        manager.registerAudioDeviceCallback(new AudioDeviceCallback() {
            @Override
            public void onAudioDevicesAdded(AudioDeviceInfo[] added) {
                ++sVersion;
                Log.i(TAG, "AudioRouter: " + added.length + " output(s) added");
            }

            @Override
            public void onAudioDevicesRemoved(AudioDeviceInfo[] removed) {
                ++sVersion;
                Log.i(TAG, "AudioRouter: " + removed.length + " output(s) removed");
            }
        }, new Handler(Looper.getMainLooper()));
    }
}
