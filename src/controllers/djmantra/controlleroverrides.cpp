#include "controllers/djmantra/controlleroverrides.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QtDebug>
#include <algorithm>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"

namespace djmantra {

namespace {

const QString kLayoutFile = QStringLiteral("controllers/djmantra/mix-ultra-layout.json");
const QString kOverridesFile = QStringLiteral("controller-overrides.json");
const QString kShiftSuffix = QStringLiteral("@shift");

int routeKey(int status, int data1) {
    return (status << 8) | data1;
}

/// A single MIDI message of the layout: {"type", "status", "data1", "mode", "lsb_data1"}
bool readMessage(const QJsonObject& msg,
        ControllerInput::Kind* pKind,
        QPair<int, int>* pMidi,
        int* pLsb) {
    if (!msg.contains(QStringLiteral("type"))) {
        return false;
    }
    const QString type = msg.value(QStringLiteral("type")).toString();
    *pMidi = qMakePair(msg.value(QStringLiteral("status")).toInt(),
            msg.value(QStringLiteral("data1")).toInt());
    if (type == QLatin1String("note")) {
        *pKind = ControllerInput::Kind::Note;
    } else {
        *pKind = msg.value(QStringLiteral("mode")).toString().startsWith(QLatin1String("relative"))
                ? ControllerInput::Kind::Relative
                : ControllerInput::Kind::Absolute;
    }
    if (pLsb && msg.contains(QStringLiteral("lsb_data1"))) {
        *pLsb = msg.value(QStringLiteral("lsb_data1")).toInt();
    }
    return true;
}

} // namespace

// static
ControllerOverrides& ControllerOverrides::instance() {
    static ControllerOverrides s_instance;
    return s_instance;
}

void ControllerOverrides::init(UserSettingsPointer pConfig) {
    m_pConfig = pConfig;
    loadLayout(QDir(pConfig->getResourcePath()).filePath(kLayoutFile));
    load();
}

void ControllerOverrides::loadLayout(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Controller layout not found:" << path;
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject image = root.value(QStringLiteral("background_image")).toObject();
    m_imagePath = QFileInfo(path).dir().filePath(image.value(QStringLiteral("file")).toString());
    m_imageWidth = image.value(QStringLiteral("w")).toDouble();
    m_imageHeight = image.value(QStringLiteral("h")).toDouble();
    const QJsonObject canvas = root.value(QStringLiteral("canvas")).toObject();
    m_canvas = QSizeF(canvas.value(QStringLiteral("w")).toDouble(34.2),
            canvas.value(QStringLiteral("h")).toDouble(19.4));
    const QJsonObject outline = root.value(QStringLiteral("outline")).toObject();
    m_outline = QRectF(outline.value(QStringLiteral("x")).toDouble(2.8),
            outline.value(QStringLiteral("y")).toDouble(1.9),
            outline.value(QStringLiteral("w")).toDouble(28.6),
            outline.value(QStringLiteral("h")).toDouble(15.7));
    m_controls.clear();
    const QJsonArray controls = root.value(QStringLiteral("controls")).toArray();
    for (const auto& value : controls) {
        const QJsonObject c = value.toObject();
        ControllerLayoutControl control;
        control.id = c.value(QStringLiteral("id")).toString();
        control.label = c.value(QStringLiteral("label")).toString();
        control.type = c.value(QStringLiteral("type")).toString();
        control.deck = c.value(QStringLiteral("deck")).toVariant().toString();
        control.shape = c.value(QStringLiteral("shape")).toString();
        control.x = c.value(QStringLiteral("x")).toDouble();
        control.y = c.value(QStringLiteral("y")).toDouble();
        control.w = c.value(QStringLiteral("w")).toDouble();
        control.h = c.value(QStringLiteral("h")).toDouble();

        const QJsonObject midi = c.value(QStringLiteral("midi")).toObject();
        const QJsonObject midiShift = c.value(QStringLiteral("midi_shift")).toObject();
        ControllerInput input;
        QPair<int, int> msg;
        if (readMessage(midi, &input.kind, &msg, &input.lsbData1)) {
            // buttons, faders, knobs, encoder turn
            input.key = control.id;
            input.label = control.label.isEmpty() ? control.id : control.label;
            input.midi.append(msg);
            ControllerInput::Kind kind;
            if (readMessage(midiShift, &kind, &msg, nullptr)) {
                input.midiShift.append(msg);
            }
            control.inputs.append(input);
        } else if (midi.contains(QStringLiteral("modes"))) {
            // pads: one note per pad mode; one function for the pad in all modes
            input.key = control.id;
            input.label = QStringLiteral("pad ") + control.label;
            const QJsonObject modes = midi.value(QStringLiteral("modes")).toObject();
            for (const auto& mode : modes) {
                if (readMessage(mode.toObject(), &input.kind, &msg, nullptr)) {
                    input.midi.append(msg);
                }
            }
            const QJsonObject shiftModes = midi.value(QStringLiteral("shift")).toObject();
            for (const auto& mode : shiftModes) {
                ControllerInput::Kind kind;
                if (readMessage(mode.toObject(), &kind, &msg, nullptr)) {
                    input.midiShift.append(msg);
                }
            }
            control.inputs.append(input);
        } else {
            // jog wheels: touch, top, ring
            const QJsonObject shiftParts = midi.value(QStringLiteral("shift")).toObject();
            for (const auto& part : {QStringLiteral("top"),
                         QStringLiteral("ring"),
                         QStringLiteral("touch")}) {
                ControllerInput partInput;
                if (!readMessage(midi.value(part).toObject(),
                            &partInput.kind,
                            &msg,
                            &partInput.lsbData1)) {
                    continue;
                }
                partInput.key = control.id + QLatin1Char('/') + part;
                partInput.label = QStringLiteral("jog ") + part;
                partInput.midi.append(msg);
                ControllerInput::Kind kind;
                if (readMessage(shiftParts.value(part).toObject(), &kind, &msg, nullptr)) {
                    partInput.midiShift.append(msg);
                }
                control.inputs.append(partInput);
            }
        }
        // the browser encoder's push
        ControllerInput press;
        if (readMessage(c.value(QStringLiteral("midi_press")).toObject(),
                    &press.kind,
                    &msg,
                    nullptr)) {
            press.key = control.id + QStringLiteral("/press");
            press.label = control.label + QStringLiteral(" press");
            press.midi.append(msg);
            ControllerInput::Kind kind;
            if (readMessage(c.value(QStringLiteral("midi_press_shift")).toObject(),
                        &kind,
                        &msg,
                        nullptr)) {
                press.midiShift.append(msg);
            }
            control.inputs.append(press);
        }
        m_controls.append(control);
    }
    qInfo() << "Controller layout:" << m_controls.size() << "controls from" << path;
}

void ControllerOverrides::load() {
    QFile file(QDir(m_pConfig->getSettingsPath()).filePath(kOverridesFile));
    const auto locker = QMutexLocker(&m_mutex);
    m_targets.clear();
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        for (auto it = root.begin(); it != root.end(); ++it) {
            m_targets.insert(it.key(), it.value().toString());
        }
    }
    rebuildRoutes();
    if (!m_targets.isEmpty()) {
        qInfo() << "Controller overrides:" << m_targets.size();
    }
}

void ControllerOverrides::save() const {
    QJsonObject root;
    for (auto it = m_targets.constBegin(); it != m_targets.constEnd(); ++it) {
        root.insert(it.key(), it.value());
    }
    QSaveFile file(QDir(m_pConfig->getSettingsPath()).filePath(kOverridesFile));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.commit();
    }
}

QString ControllerOverrides::target(const QString& inputKey, bool shift) const {
    const auto locker = QMutexLocker(&m_mutex);
    return m_targets.value(shift ? inputKey + kShiftSuffix : inputKey);
}

void ControllerOverrides::setTarget(const QString& inputKey, bool shift, const QString& configKey) {
    const auto locker = QMutexLocker(&m_mutex);
    m_targets.insert(shift ? inputKey + kShiftSuffix : inputKey, configKey);
    rebuildRoutes();
    save();
    qInfo() << "Controller override:" << inputKey << (shift ? "(shift)" : "") << "->" << configKey;
}

void ControllerOverrides::clear(const QString& inputKey, bool shift) {
    const auto locker = QMutexLocker(&m_mutex);
    m_targets.remove(shift ? inputKey + kShiftSuffix : inputKey);
    rebuildRoutes();
    save();
    qInfo() << "Controller override removed:" << inputKey << (shift ? "(shift)" : "");
}

void ControllerOverrides::clearAll() {
    const auto locker = QMutexLocker(&m_mutex);
    m_targets.clear();
    rebuildRoutes();
    save();
}

int ControllerOverrides::count() const {
    const auto locker = QMutexLocker(&m_mutex);
    return static_cast<int>(m_targets.size());
}

// m_mutex held
void ControllerOverrides::rebuildRoutes() {
    m_routes.clear();
    for (const auto& control : std::as_const(m_controls)) {
        for (const auto& input : control.inputs) {
            for (bool shift : {false, true}) {
                const QString target = m_targets.value(shift ? input.key + kShiftSuffix : input.key);
                if (target.isEmpty()) {
                    continue;
                }
                Route route;
                route.inputKey = input.key;
                route.target = target;
                route.kind = input.kind;
                route.step = control.type == QLatin1String("jog") ? 0.004 : 0.02;
                for (const auto& midi : shift ? input.midiShift : input.midi) {
                    m_routes.insert(routeKey(midi.first, midi.second), route);
                    // note off (0x8n) for notes
                    if (input.kind == ControllerInput::Kind::Note) {
                        m_routes.insert(routeKey((midi.first & 0x0F) | 0x80, midi.second), route);
                    }
                    if (input.lsbData1 >= 0) {
                        Route lsb = route;
                        lsb.lsb = true;
                        m_routes.insert(routeKey(midi.first, input.lsbData1), lsb);
                    }
                }
            }
        }
    }
}

void ControllerOverrides::setMidiObserver(std::function<void(int, int)> observer) {
    const auto locker = QMutexLocker(&m_mutex);
    m_observer = std::move(observer);
}

bool ControllerOverrides::findInput(int status, int data1, int* pControl, int* pInput) const {
    // A note off (0x8n) belongs to the note on (0x9n)
    const int noteOn = (status & 0xF0) == 0x80 ? (status & 0x0F) | 0x90 : status;
    for (int c = 0; c < m_controls.size(); ++c) {
        const auto& inputs = m_controls[c].inputs;
        for (int i = 0; i < inputs.size(); ++i) {
            const auto& input = inputs[i];
            for (const auto* pList : {&input.midi, &input.midiShift}) {
                for (const auto& midi : *pList) {
                    if (midi.first == noteOn &&
                            (midi.second == data1 || input.lsbData1 == data1)) {
                        *pControl = c;
                        *pInput = i;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ControllerOverrides::handleMidi(
        unsigned char status, unsigned char data1, unsigned char value) {
    Route route;
    {
        const auto locker = QMutexLocker(&m_mutex);
        if (m_observer && !((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && value == 0))) {
            // presses and moves select the control in the virtual controller
            m_observer(status, data1);
        }
        if (m_routes.isEmpty()) {
            return false;
        }
        const auto it = m_routes.constFind(routeKey(status, data1));
        if (it == m_routes.constEnd()) {
            return false;
        }
        route = it.value();
        if (route.kind == ControllerInput::Kind::Note && (status & 0xF0) == 0x80) {
            value = 0;
        }
        if (route.kind == ControllerInput::Kind::Absolute) {
            if (route.lsb) {
                const int msb = m_msb.value(route.inputKey, 0);
                apply(route, msb * 128 + value);
                return true;
            }
            m_msb.insert(route.inputKey, value);
            apply(route, value * 128);
            return true;
        }
    }
    apply(route, value);
    return true;
}

void ControllerOverrides::apply(const Route& route, int value) {
    const ConfigKey key = ConfigKey::parseCommaSeparated(route.target);
    ControlObject* pControl = ControlObject::getControl(key, ControlFlag::NoWarnIfMissing);
    if (!pControl) {
        return;
    }
    auto* pButton = qobject_cast<ControlPushButton*>(pControl);
    auto pressButton = [pButton](bool pressed) {
        switch (pButton->getButtonMode()) {
        case ControlPushButton::TOGGLE:
        case ControlPushButton::POWERWINDOW:
        case ControlPushButton::LONGPRESSLATCHING:
            if (pressed) {
                pButton->set(pButton->get() > 0 ? 0.0 : 1.0);
            }
            break;
        case ControlPushButton::TRIGGER:
            if (pressed) {
                pButton->set(1.0);
            }
            break;
        default: // PUSH
            pButton->set(pressed ? 1.0 : 0.0);
            break;
        }
    };
    switch (route.kind) {
    case ControllerInput::Kind::Note: {
        const bool pressed = value > 0;
        if (pButton) {
            pressButton(pressed);
        } else if (pressed) {
            // a button on a fader or knob: full, then back to its default
            if (pControl->get() == pControl->defaultValue()) {
                pControl->setParameter(1.0);
            } else {
                pControl->reset();
            }
        }
        break;
    }
    case ControllerInput::Kind::Absolute: {
        const double parameter = std::clamp(value / 16383.0, 0.0, 1.0);
        if (pButton) {
            // a fader or knob on a button: pressed in the upper half
            const bool pressed = parameter > 0.5;
            if (m_pressed.value(route.inputKey, false) != pressed) {
                m_pressed.insert(route.inputKey, pressed);
                pressButton(pressed);
            }
        } else {
            pControl->setParameter(parameter);
        }
        break;
    }
    case ControllerInput::Kind::Relative: {
        const int ticks = value < 64 ? value : value - 128;
        if (ticks == 0) {
            break;
        }
        if (pButton) {
            // each tick is a press
            pressButton(true);
            pressButton(false);
        } else {
            pControl->setParameter(
                    std::clamp(pControl->getParameter() + ticks * route.step, 0.0, 1.0));
        }
        break;
    }
    }
}

} // namespace djmantra
