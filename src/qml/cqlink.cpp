#include "qml/cqlink.h"

#include <QtDebug>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "moc_cqlink.cpp"

namespace djmantra {

namespace {

// CQ MIDI Protocol V1.2, page 15: dB -> VC VF (as one 14-bit value)
constexpr std::array<std::pair<double, int>, 10> kLevels = {{
        {-90.0, 0},
        {-40.0, 0x0F * 128 + 0x40},
        {-30.0, 0x1F * 128 + 0x00},
        {-20.0, 0x2E * 128 + 0x40},
        {-10.0, 0x3E * 128 + 0x00},
        {-5.0, 0x4E * 128 + 0x40},
        {-3.0, 0x56 * 128 + 0x40},
        {0.0, 0x62 * 128 + 0x00},
        {5.0, 0x73 * 128 + 0x40},
        {10.0, 0x7F * 128 + 0x40},
}};

} // namespace

CqLink::CqLink(QObject* pParent)
        : QObject(pParent),
          m_status(QStringLiteral("not connected")) {
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        setStatus(QStringLiteral("connected to ") + m_host);
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        setStatus(QStringLiteral("disconnected"));
        if (!m_host.isEmpty()) {
            m_reconnect.start();
        }
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        setStatus(QStringLiteral("no connection: ") + m_socket.errorString());
        if (!m_host.isEmpty()) {
            m_reconnect.start();
        }
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, [this] {
        // the CQ sends NRPN when its own parameters change; kept for later (the app
        // could follow the mixer's faders)
        m_socket.readAll();
    });
    m_reconnect.setSingleShot(true);
    m_reconnect.setInterval(5000);
    connect(&m_reconnect, &QTimer::timeout, this, [this] {
        if (!m_host.isEmpty() && m_socket.state() == QAbstractSocket::UnconnectedState) {
            m_socket.connectToHost(m_host, kPort);
        }
    });
    m_tapTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_tapTimer, &QTimer::timeout, this, &CqLink::tapOnce);
}

void CqLink::tapOnce() {
    if (m_tapsLeft-- <= 0) {
        m_tapTimer.stop();
        return;
    }
    const char key = static_cast<char>(0x30 + m_tapKey - 1);
    send(QByteArray("\x90", 1) + key + QByteArray("\x7F", 1));
    QTimer::singleShot(30, Qt::PreciseTimer, this, [this, key] {
        send(QByteArray("\x80", 1) + key + QByteArray("\x00", 1));
    });
}

void CqLink::connectTo(const QString& host) {
    m_host = host.trimmed();
    m_socket.abort();
    if (m_host.isEmpty()) {
        setStatus(QStringLiteral("not connected"));
        return;
    }
    setStatus(QStringLiteral("connecting to ") + m_host);
    m_socket.connectToHost(m_host, kPort);
}

void CqLink::disconnectFrom() {
    m_host.clear();
    m_reconnect.stop();
    m_socket.disconnectFromHost();
    setStatus(QStringLiteral("not connected"));
}

bool CqLink::isConnected() const {
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void CqLink::setStatus(const QString& status) {
    if (status != m_status) {
        m_status = status;
        qInfo().noquote() << "CQ:" << status;
        emit statusChanged();
    }
}

void CqLink::send(const QByteArray& bytes) {
    if (isConnected()) {
        m_socket.write(bytes);
        m_socket.flush();
    }
}

void CqLink::nrpn(int msb, int lsb, int value14) {
    const auto b = [](int v) {
        return static_cast<char>(v & 0x7F);
    };
    QByteArray bytes;
    bytes.append('\xB0').append('\x63').append(b(msb));
    bytes.append('\xB0').append('\x62').append(b(lsb));
    bytes.append('\xB0').append('\x06').append(b(value14 >> 7));
    bytes.append('\xB0').append('\x26').append(b(value14));
    send(bytes);
}

int CqLink::levelValue(double dB) {
    if (!std::isfinite(dB) || dB <= kLevels.front().first) {
        return 0;
    }
    if (dB >= kLevels.back().first) {
        return kLevels.back().second;
    }
    for (size_t i = 1; i < kLevels.size(); ++i) {
        if (dB <= kLevels[i].first) {
            const auto& [d0, v0] = kLevels[i - 1];
            const auto& [d1, v1] = kLevels[i];
            return static_cast<int>(std::lround(v0 + (v1 - v0) * (dB - d0) / (d1 - d0)));
        }
    }
    return kLevels.back().second;
}

void CqLink::setLevel(int msb, int lsb, double dB) {
    nrpn(msb, lsb, levelValue(dB));
    qInfo().noquote() << "CQ: level" << QString::number(msb, 16) << QString::number(lsb, 16)
                      << dB << "dB";
}

void CqLink::setMute(int msb, int lsb, bool muted) {
    nrpn(msb, lsb, muted ? 1 : 0);
    qInfo().noquote() << "CQ: mute" << QString::number(msb, 16) << QString::number(lsb, 16)
                      << muted;
}

void CqLink::tapTempo(int softKey, double bpm, int count) {
    if (softKey < 1 || softKey > 3 || bpm < 40 || bpm > 250 || !isConnected()) {
        return;
    }
    m_tapKey = softKey;
    m_tapsLeft = count;
    m_tapTimer.start(static_cast<int>(std::lround(60000.0 / bpm)));
    tapOnce(); // the first tap now
    qInfo().noquote() << "CQ: tap tempo" << bpm << "BPM on soft key" << softKey;
}

} // namespace djmantra
