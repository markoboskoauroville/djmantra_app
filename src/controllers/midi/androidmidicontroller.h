#pragma once

#include <QByteArray>
#include <QMutex>
#include <QQueue>

#include <functional>

#include "controllers/controllerenumerator.h"
#include "controllers/midi/midicontroller.h"

/// Splits a MIDI byte stream (as Android delivers it, e.g. with running
/// status over Bluetooth) into short messages and SysEx messages.
class MidiStreamParser {
  public:
    using ShortFn = std::function<void(unsigned char, unsigned char, unsigned char)>;
    using SysexFn = std::function<void(const QByteArray&)>;
    void feed(const QByteArray& data, const ShortFn& onShort, const SysexFn& onSysex);

  private:
    QByteArray m_sysex;
    bool m_inSysex = false;
    unsigned char m_runningStatus = 0;
    // The message being received (it may come in several packets)
    unsigned char m_status = 0;
    unsigned char m_data[2] = {0, 0};
    int m_count = 0;
    int m_needed = 0;
};

/// DJ Mantra: a DJ controller over Android MIDI (USB or Bluetooth LE).
///
/// The connection itself lives in Java (packaging/android/package/src/com/
/// djmantra/app/MidiBridge.java): it finds the controller over USB or scans
/// for it over Bluetooth, reconnects when it comes back and keeps the screen
/// on while connected. This controller stays the same object across
/// reconnects, so its mapping stays loaded; after every (re)connect it asks
/// the controller for all knob positions (B0 7F 7F) and lets the mapping
/// refresh its LEDs (djmantraControllerConnected() in the script, if any).
class AndroidMidiController : public MidiController {
    Q_OBJECT
  public:
    explicit AndroidMidiController(const QString& name);
    ~AndroidMidiController() override;

    bool isPolling() const override {
        return true;
    }

    /// Called from Java's MIDI thread
    void receiveFromJava(const QByteArray& data);
    void connectedFromJava(bool connected, const QString& transport);

  protected:
    bool poll() override;
    void sendShortMsg(unsigned char status, unsigned char byte1, unsigned char byte2) override;
    void sendBytes(const QByteArray& data) override;

  private:
    int open() override;
    int close() override;

    void handleConnected();
    void processBytes(const QByteArray& data);

    QMutex m_mutex;
    QQueue<QByteArray> m_incoming;
    QAtomicInt m_connectedEvents;
    MidiStreamParser m_parser;
};

class AndroidMidiEnumerator : public ControllerEnumerator {
    Q_OBJECT
  public:
    AndroidMidiEnumerator();
    ~AndroidMidiEnumerator() override;

    QList<Controller*> queryDevices() override;
    bool needPolling() override {
        return true;
    }

  private:
    QList<Controller*> m_devices;
};
