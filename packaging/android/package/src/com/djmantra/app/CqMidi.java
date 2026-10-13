package com.djmantra.app;

import android.content.Context;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import java.io.IOException;
import java.util.Locale;

/**
 * The Allen & Heath CQ over USB MIDI (round 37: the CQ-12T on the phone's USB is also a
 * MIDI device, "Allen&Heath Ltd CQ12T"): the same published CQ MIDI messages as over the
 * network (qml/cqlink.h), only sent. The native side polls state() and calls open() until
 * it is open.
 */
public final class CqMidi {
    private static final String TAG = "DJMantraCQ";
    private static final Object LOCK = new Object();
    private static HandlerThread sThread;
    private static Handler sHandler;
    private static MidiDevice sDevice;
    private static MidiInputPort sPort;
    private static boolean sOpening;
    private static String sName = "";

    private CqMidi() {
    }

    /** Looks for a USB MIDI device named like the CQ and opens it (once at a time). */
    public static void open(Context context) {
        synchronized (LOCK) {
            if (sPort != null || sOpening) {
                return;
            }
            MidiManager midi = (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
            if (midi == null) {
                return;
            }
            if (sThread == null) {
                sThread = new HandlerThread("CqMidi");
                sThread.start();
                sHandler = new Handler(sThread.getLooper());
            }
            for (MidiDeviceInfo info : midi.getDevices()) {
                Bundle p = info.getProperties();
                String name = p.getString(MidiDeviceInfo.PROPERTY_NAME, "")
                        + " " + p.getString(MidiDeviceInfo.PROPERTY_PRODUCT, "");
                String lower = name.toLowerCase(Locale.ROOT);
                if (info.getInputPortCount() > 0
                        && (lower.contains("cq") || lower.contains("allen"))) {
                    sOpening = true;
                    sName = name.trim();
                    Log.i(TAG, "Opening " + sName);
                    midi.openDevice(info, device -> {
                        synchronized (LOCK) {
                            sOpening = false;
                            if (device == null) {
                                Log.w(TAG, "Could not open " + sName);
                                return;
                            }
                            sDevice = device;
                            sPort = device.openInputPort(0);
                            Log.i(TAG, sPort != null ? "Open: " + sName : "No input port");
                        }
                    }, sHandler);
                    return;
                }
            }
        }
    }

    /** 1 = open, 0 = not (yet). */
    public static int state() {
        synchronized (LOCK) {
            return sPort != null ? 1 : 0;
        }
    }

    public static String name() {
        synchronized (LOCK) {
            return sName;
        }
    }

    public static boolean send(byte[] data) {
        synchronized (LOCK) {
            if (sPort == null) {
                return false;
            }
            try {
                sPort.send(data, 0, data.length);
                return true;
            } catch (IOException e) {
                Log.w(TAG, "send failed, closing: " + e);
                closeLocked();
                return false;
            }
        }
    }

    public static void close() {
        synchronized (LOCK) {
            closeLocked();
        }
    }

    private static void closeLocked() {
        try {
            if (sPort != null) {
                sPort.close();
            }
            if (sDevice != null) {
                sDevice.close();
            }
        } catch (IOException e) {
            Log.w(TAG, "close: " + e);
        }
        sPort = null;
        sDevice = null;
    }
}
