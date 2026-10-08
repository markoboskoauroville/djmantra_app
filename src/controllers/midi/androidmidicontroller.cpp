#include "controllers/midi/androidmidicontroller.h"

#include "controllers/scripting/legacy/controllerscriptenginelegacy.h"
#include "moc_androidmidicontroller.cpp"
#include "util/time.h"
#include "util/logger.h"

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QJniObject>
#endif

namespace {

const mixxx::Logger kLogger("DJMantraMIDI");

// The controllers DJ Mantra connects to on Android (matched by name)
const QString kMixUltra = QStringLiteral("DJControl Mix Ultra");

constexpr const char* kBridge = "com/djmantra/app/MidiBridge";

AndroidMidiController* s_pController = nullptr;

int messageLength(unsigned char status) {
    switch (status & 0xF0) {
    case 0xC0:
    case 0xD0:
        return 2;
    case 0xF0:
        switch (status) {
        case 0xF1:
        case 0xF3:
            return 2;
        case 0xF2:
            return 3;
        default:
            return 1;
        }
    default:
        return 3;
    }
}

} // namespace

#if defined(Q_OS_ANDROID)
extern "C" {

JNIEXPORT void JNICALL Java_com_djmantra_app_MidiBridge_nativeReceive(JNIEnv* env,
        jobject,
        jlong handle,
        jbyteArray data,
        jint offset,
        jint count,
        jlong) {
    auto* pController = reinterpret_cast<AndroidMidiController*>(handle);
    if (!pController || pController != s_pController || count <= 0) {
        return;
    }
    QByteArray bytes(count, Qt::Uninitialized);
    env->GetByteArrayRegion(data, offset, count, reinterpret_cast<jbyte*>(bytes.data()));
    pController->receiveFromJava(bytes);
}

JNIEXPORT void JNICALL Java_com_djmantra_app_MidiBridge_nativeConnected(
        JNIEnv*, jobject, jlong handle, jboolean connected, jstring transport) {
    auto* pController = reinterpret_cast<AndroidMidiController*>(handle);
    if (!pController || pController != s_pController) {
        return;
    }
    pController->connectedFromJava(connected,
            QJniObject(transport).toString());
}

} // extern "C"
#endif

AndroidMidiController::AndroidMidiController(const QString& name)
        : MidiController(name) {
    setInputDevice(true);
    setOutputDevice(true);
}

AndroidMidiController::~AndroidMidiController() {
    if (isOpen()) {
        close();
    }
}

int AndroidMidiController::open() {
    if (isOpen()) {
        return -1;
    }
    m_parser = MidiStreamParser();
    startEngine();
    applyMapping();
    setOpen(true);
    s_pController = this;
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(kBridge,
            "start",
            "(Landroid/content/Context;Ljava/lang/String;J)V",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(getName()).object<jstring>(),
            static_cast<jlong>(reinterpret_cast<qintptr>(this)));
#endif
    kLogger.info() << "Looking for" << getName() << "over USB and Bluetooth";
    return 0;
}

int AndroidMidiController::close() {
    if (!isOpen()) {
        return -1;
    }
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(kBridge, "stop", "()V");
#endif
    s_pController = nullptr;
    stopEngine();
    MidiController::close();
    setOpen(false);
    return 0;
}

void AndroidMidiController::receiveFromJava(const QByteArray& data) {
    const auto locker = QMutexLocker(&m_mutex);
    m_incoming.enqueue(data);
}

void AndroidMidiController::connectedFromJava(bool connected, const QString& transport) {
    kLogger.info() << getName() << (connected ? "connected over" : "disconnected from")
                   << transport;
    if (connected) {
        m_connectedEvents.ref();
    }
}

bool AndroidMidiController::poll() {
    if (m_connectedEvents.fetchAndStoreRelaxed(0) > 0) {
        handleConnected();
    }
    QQueue<QByteArray> incoming;
    {
        const auto locker = QMutexLocker(&m_mutex);
        incoming.swap(m_incoming);
    }
    for (const auto& data : std::as_const(incoming)) {
        processBytes(data);
    }
    return !incoming.isEmpty();
}

void AndroidMidiController::handleConnected() {
    // The controller sends every knob and fader position
    sendShortMsg(0xB0, 0x7F, 0x7F);
    // Let the mapping light the LEDs again (they are dark after a reconnect)
    if (getScriptEngine() && getScriptEngine()->jsEngine()) {
        getScriptEngine()->jsEngine()->evaluate(QStringLiteral(
                "if (typeof djmantraControllerConnected === 'function') "
                "{ djmantraControllerConnected(); }"));
    }
}

void MidiStreamParser::feed(
        const QByteArray& data, const ShortFn& onShort, const SysexFn& onSysex) {
    for (const char c : data) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte >= 0xF8) {
            onShort(byte, 0, 0); // real-time, may come anywhere
            continue;
        }
        if (m_inSysex) {
            m_sysex.append(c);
            if (byte == 0xF7) {
                m_inSysex = false;
                onSysex(m_sysex);
                m_sysex.clear();
            }
            continue;
        }
        if (byte == 0xF0) {
            m_inSysex = true;
            m_sysex = QByteArray(1, c);
            m_status = 0;
            continue;
        }
        if (byte & 0x80) {
            m_status = byte;
            m_count = 0;
            m_needed = messageLength(byte) - 1;
            m_runningStatus = byte < 0xF0 ? byte : 0;
            if (m_needed == 0) {
                onShort(byte, 0, 0);
                m_status = 0;
            }
            continue;
        }
        // A data byte
        if (m_status == 0) {
            if (m_runningStatus == 0) {
                continue; // data without status: skip
            }
            m_status = m_runningStatus;
            m_count = 0;
            m_needed = messageLength(m_status) - 1;
        }
        m_data[m_count++] = byte;
        if (m_count == m_needed) {
            onShort(m_status, m_data[0], m_needed > 1 ? m_data[1] : 0);
            // The next data bytes use running status
            m_status = 0;
            m_count = 0;
        }
    }
}

void AndroidMidiController::processBytes(const QByteArray& data) {
    const auto timestamp = mixxx::Time::elapsed();
    m_parser.feed(
            data,
            [this, timestamp](unsigned char status, unsigned char data1, unsigned char data2) {
                receivedShortMessage(status, data1, data2, timestamp);
            },
            [this, timestamp](const QByteArray& sysex) {
                receive(sysex, timestamp);
            });
}

void AndroidMidiController::sendShortMsg(
        unsigned char status, unsigned char byte1, unsigned char byte2) {
    QByteArray data;
    data.append(static_cast<char>(status));
    const int length = messageLength(status);
    if (length > 1) {
        data.append(static_cast<char>(byte1));
    }
    if (length > 2) {
        data.append(static_cast<char>(byte2));
    }
    sendBytes(data);
}

void AndroidMidiController::sendBytes(const QByteArray& data) {
#if defined(Q_OS_ANDROID)
    QJniEnvironment env;
    jbyteArray array = env->NewByteArray(data.size());
    env->SetByteArrayRegion(array, 0, data.size(), reinterpret_cast<const jbyte*>(data.constData()));
    QJniObject::callStaticMethod<void>(kBridge, "send", "([B)V", array);
    env->DeleteLocalRef(array);
#else
    Q_UNUSED(data);
#endif
}

AndroidMidiEnumerator::AndroidMidiEnumerator() = default;

AndroidMidiEnumerator::~AndroidMidiEnumerator() {
    qDeleteAll(m_devices);
}

QList<Controller*> AndroidMidiEnumerator::queryDevices() {
    // One controller per supported model: present from the start, it connects
    // whenever the device shows up (USB or Bluetooth)
    if (m_devices.isEmpty()) {
        m_devices.append(new AndroidMidiController(kMixUltra));
    }
    return m_devices;
}
