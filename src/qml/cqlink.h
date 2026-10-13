#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

/// DJ Mantra: the Allen & Heath CQ mixer over the network (Marko 13.10.2026: "trigger
/// the CQ's delay through the app ... read the BPM from the app"). Allen & Heath's
/// published CQ MIDI Protocol V1.2 only (testing/results/20261013_cq12t/midi.md):
/// MIDI over TCP on port 51325, one connection at a time, MIDI channel 1; NRPN
/// `B0 63 MSB B0 62 LSB B0 06 VC B0 26 VF` for levels / mutes / pans; soft keys 1-3 =
/// notes 0x30-0x32. No message sets a delay time, so the tempo goes in as taps on a
/// soft key set to the delay's Tap Tempo. The host "usb" = the same messages over the CQ's
/// own USB MIDI (Android, CqMidi.java), when the CQ is plugged into the phone.
namespace djmantra {

class CqLink : public QObject {
    Q_OBJECT
  public:
    static constexpr quint16 kPort = 51325;

    explicit CqLink(QObject* pParent = nullptr);

    void connectTo(const QString& host);
    void disconnectFrom();
    bool isConnected() const;
    QString status() const {
        return m_status;
    }

    /// A level in dB (input -> mix, a master ...): -inf below -89 dB
    void setLevel(int msb, int lsb, double dB);
    void setMute(int msb, int lsb, bool muted);
    /// `count` taps of soft key 1-3 at `bpm` (the delay's Tap Tempo follows them)
    void tapTempo(int softKey, double bpm, int count = 4);

    /// The CQ's 14-bit level for a dB value (the PDF's table, interpolated)
    static int levelValue(double dB);

  signals:
    void statusChanged();

  private:
    void tapOnce();
    void send(const QByteArray& bytes);
    void nrpn(int msb, int lsb, int value14);
    void setStatus(const QString& status);
    bool usb() const {
        return m_host.compare(QStringLiteral("usb"), Qt::CaseInsensitive) == 0;
    }
    void pollUsb();

    QTcpSocket m_socket;
    QString m_host;
    QString m_status;
    QTimer m_reconnect;
    QTimer m_usbPoll;
    bool m_usbOpen = false;
    QTimer m_tapTimer;
    int m_tapsLeft = 0;
    int m_tapKey = 0;
};

} // namespace djmantra
