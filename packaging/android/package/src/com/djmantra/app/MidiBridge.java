package com.djmantra.app;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.SharedPreferences;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.WindowManager;

import java.io.IOException;
import java.util.Collections;
import java.util.Locale;

/**
 * One DJ controller over Android MIDI, for the native AndroidMidiController
 * (src/controllers/midi/androidmidicontroller.cpp).
 *
 * USB: a MidiManager device whose name contains the controller name.
 * Bluetooth LE: scan for the BLE MIDI service and open the device with
 * MidiManager.openBluetoothDevice(). The user never pairs it in Settings
 * (a pairing there hides it from apps). The last Bluetooth address is
 * remembered and reconnected when the device comes back.
 *
 * Every incoming message is logged with the tag DJMantraMIDI.
 */
public final class MidiBridge {
    private static final String TAG = "DJMantraMIDI";
    private static final ParcelUuid BLE_MIDI_SERVICE =
            ParcelUuid.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
    private static final String PREFS = "djmantra_midi";
    private static final long RETRY_MS = 3000;

    private static MidiBridge sInstance;

    private final Context mContext;
    private final String mName;
    private final long mNativeHandle;
    private final MidiManager mMidi;
    private final Handler mHandler;
    private MidiDevice mDevice;
    private MidiInputPort mToDevice;
    private MidiOutputPort mFromDevice;
    private String mTransport = "";
    private boolean mScanning;
    private boolean mStopped;

    private native void nativeReceive(long handle, byte[] data, int offset, int count, long timestamp);
    private native void nativeConnected(long handle, boolean connected, String transport);

    private MidiBridge(Context context, String name, long nativeHandle) {
        mContext = context.getApplicationContext();
        mName = name.toLowerCase(Locale.ROOT);
        mNativeHandle = nativeHandle;
        mMidi = mContext.getSystemService(MidiManager.class);
        HandlerThread thread = new HandlerThread("DJMantraMIDI");
        thread.start();
        mHandler = new Handler(thread.getLooper());
    }

    /** Starts looking for the controller (USB, then Bluetooth). */
    public static synchronized void start(Context context, String name, long nativeHandle) {
        if (sInstance != null) {
            return;
        }
        sInstance = new MidiBridge(context, name, nativeHandle);
        sInstance.mHandler.post(sInstance::begin);
    }

    public static synchronized void stop() {
        if (sInstance != null) {
            MidiBridge bridge = sInstance;
            sInstance = null;
            bridge.mHandler.post(bridge::end);
        }
    }

    /** Sends MIDI bytes to the controller. */
    public static void send(byte[] data) {
        MidiBridge bridge = sInstance;
        if (bridge == null) {
            return;
        }
        MidiInputPort port = bridge.mToDevice;
        if (port == null) {
            return;
        }
        try {
            port.send(data, 0, data.length);
        } catch (IOException e) {
            Log.w(TAG, "send failed: " + e);
        }
    }

    private void begin() {
        if (mMidi == null) {
            Log.w(TAG, "No MIDI on this device");
            return;
        }
        mMidi.registerDeviceCallback(new MidiManager.DeviceCallback() {
            @Override
            public void onDeviceAdded(MidiDeviceInfo info) {
                if (mDevice == null && matches(info)) {
                    open(info);
                }
            }

            @Override
            public void onDeviceRemoved(MidiDeviceInfo info) {
                if (mDevice != null && mDevice.getInfo().getId() == info.getId()) {
                    Log.i(TAG, "Disconnected (" + mTransport + ")");
                    closeDevice();
                    mHandler.postDelayed(MidiBridge.this::connect, RETRY_MS);
                }
            }
        }, mHandler);
        connect();
    }

    private void end() {
        mStopped = true;
        stopScan();
        closeDevice();
    }

    /** USB (or an already connected Bluetooth device) first, then a Bluetooth scan. */
    private void connect() {
        if (mStopped || mDevice != null) {
            return;
        }
        for (MidiDeviceInfo info : mMidi.getDevices()) {
            if (matches(info)) {
                open(info);
                return;
            }
        }
        startScan();
    }

    private boolean matches(MidiDeviceInfo info) {
        if (info.getInputPortCount() < 1 || info.getOutputPortCount() < 1) {
            return false;
        }
        Bundle properties = info.getProperties();
        String name = properties.getString(MidiDeviceInfo.PROPERTY_NAME, "");
        String product = properties.getString(MidiDeviceInfo.PROPERTY_PRODUCT, "");
        return name.toLowerCase(Locale.ROOT).contains(mName)
                || product.toLowerCase(Locale.ROOT).contains(mName);
    }

    private void open(MidiDeviceInfo info) {
        Log.i(TAG, "Opening " + info.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME)
                + " type " + info.getType());
        mMidi.openDevice(info, device -> {
            if (device == null) {
                Log.w(TAG, "Could not open the device");
                mHandler.postDelayed(this::connect, RETRY_MS);
                return;
            }
            attach(device, info.getType() == MidiDeviceInfo.TYPE_BLUETOOTH ? "bluetooth" : "usb");
        }, mHandler);
    }

    private void attach(MidiDevice device, String transport) {
        stopScan();
        mDevice = device;
        mTransport = transport;
        // Port 0 both ways (the Mix Ultra has one of each)
        mToDevice = device.openInputPort(0);
        mFromDevice = device.openOutputPort(0);
        if (mFromDevice != null) {
            mFromDevice.connect(new MidiReceiver() {
                @Override
                public void onSend(byte[] data, int offset, int count, long timestamp) {
                    Log.i(TAG, "in " + hex(data, offset, count));
                    nativeReceive(mNativeHandle, data, offset, count, timestamp);
                }
            });
        }
        Log.i(TAG, "Connected (" + transport + ")");
        keepScreenOn(true);
        nativeConnected(mNativeHandle, true, transport);
    }

    private void closeDevice() {
        boolean wasConnected = mDevice != null;
        try {
            if (mFromDevice != null) {
                mFromDevice.close();
            }
            if (mToDevice != null) {
                mToDevice.close();
            }
            if (mDevice != null) {
                mDevice.close();
            }
        } catch (IOException e) {
            Log.w(TAG, "close: " + e);
        }
        mFromDevice = null;
        mToDevice = null;
        mDevice = null;
        if (wasConnected) {
            keepScreenOn(false);
            nativeConnected(mNativeHandle, false, mTransport);
        }
    }

    // --- Bluetooth LE -------------------------------------------------------

    private BluetoothLeScanner scanner() {
        BluetoothManager manager = mContext.getSystemService(BluetoothManager.class);
        BluetoothAdapter adapter = manager != null ? manager.getAdapter() : null;
        if (adapter == null || !adapter.isEnabled()) {
            return null;
        }
        return adapter.getBluetoothLeScanner();
    }

    private final ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            String name = result.getScanRecord() != null ? result.getScanRecord().getDeviceName() : null;
            String last = prefs().getString("last_address", "");
            boolean wanted = device.getAddress().equals(last)
                    || (name != null && name.toLowerCase(Locale.ROOT).contains(mName));
            if (!wanted || mDevice != null) {
                return;
            }
            Log.i(TAG, "Found " + name + " " + device.getAddress() + " over Bluetooth");
            stopScan();
            prefs().edit().putString("last_address", device.getAddress()).apply();
            mMidi.openBluetoothDevice(device, opened -> {
                if (opened == null) {
                    Log.w(TAG, "Could not open the Bluetooth device");
                    mHandler.postDelayed(MidiBridge.this::connect, RETRY_MS);
                    return;
                }
                attach(opened, "bluetooth");
            }, mHandler);
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.w(TAG, "Bluetooth scan failed: " + errorCode);
            mScanning = false;
            mHandler.postDelayed(MidiBridge.this::connect, RETRY_MS);
        }
    };

    private void startScan() {
        if (mScanning || mStopped) {
            return;
        }
        BluetoothLeScanner scanner = scanner();
        if (scanner == null) {
            // Bluetooth off or not allowed: try again later (USB still works meanwhile)
            mHandler.postDelayed(this::connect, RETRY_MS * 3);
            return;
        }
        try {
            ScanFilter filter = new ScanFilter.Builder().setServiceUuid(BLE_MIDI_SERVICE).build();
            ScanSettings settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
            scanner.startScan(Collections.singletonList(filter), settings, mScanCallback);
            mScanning = true;
            Log.i(TAG, "Looking for the controller over Bluetooth");
        } catch (SecurityException e) {
            Log.w(TAG, "Bluetooth permission missing: " + e);
            mHandler.postDelayed(this::connect, RETRY_MS * 3);
        }
    }

    private void stopScan() {
        if (!mScanning) {
            return;
        }
        mScanning = false;
        BluetoothLeScanner scanner = scanner();
        if (scanner != null) {
            try {
                scanner.stopScan(mScanCallback);
            } catch (SecurityException e) {
                Log.w(TAG, "stopScan: " + e);
            }
        }
    }

    // --- helpers ------------------------------------------------------------

    private SharedPreferences prefs() {
        return mContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    /** Keeps the screen on while a controller is connected. */
    private void keepScreenOn(boolean on) {
        Activity activity = org.qtproject.qt.android.QtNative.activity();
        if (activity == null) {
            return;
        }
        activity.runOnUiThread(() -> {
            if (on) {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            } else {
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });
    }

    private static String hex(byte[] data, int offset, int count) {
        StringBuilder text = new StringBuilder();
        for (int i = 0; i < count; i++) {
            if (i > 0) {
                text.append(' ');
            }
            text.append(String.format(Locale.ROOT, "%02X", data[offset + i] & 0xFF));
        }
        return text.toString();
    }
}
