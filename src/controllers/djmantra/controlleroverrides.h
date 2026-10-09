#pragma once

#include <QHash>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QString>

#include "preferences/usersettings.h"

namespace djmantra {

/// One input of a controller control: what can get its own function.
/// A button, pad, fader or knob has one; a jog wheel has three (touch, top,
/// ring); the browser encoder has two (turn, press).
struct ControllerInput {
    enum class Kind {
        Note,     // buttons, pads, touch: press 127, release 0
        Absolute, // faders, knobs: 0..127 (+ LSB)
        Relative, // encoders, jog: 1 = clockwise, 127 = counter-clockwise
    };
    QString key;   // "<control id>" or "<control id>/<part>"
    QString label; // e.g. "jog top"
    Kind kind = Kind::Note;
    QList<QPair<int, int>> midi;      // (status, data1): pads send one per pad mode
    QList<QPair<int, int>> midiShift; // the same with SHIFT held
    int lsbData1 = -1;                // 14-bit faders and knobs
};

/// One control of the controller, from the layout file
/// (res/controllers/djmantra/mix-ultra-layout.json, made from the
/// Controller Mapper app's layout).
struct ControllerLayoutControl {
    QString id;
    QString label;
    QString type; // button, pad, fader, knob, encoder, jog, led
    QString deck; // 1, 2 or master
    QString shape;
    double x = 0; // centre, in layout units
    double y = 0;
    double w = 0;
    double h = 0;
    QList<ControllerInput> inputs;
};

/// DJ Mantra: any controller button, pad, fader, knob, encoder or jog can do
/// any function of the app, chosen in the virtual controller
/// (DlgControllerRemap). An override replaces the controller mapping for that
/// input; the others keep the mapping.
///
/// The choices are stored in <settings>/controller-overrides.json as
/// { "<input key>": "[Group],item", "<input key>@shift": "[Group],item" }.
class ControllerOverrides {
  public:
    static ControllerOverrides& instance();

    /// Reads the layout and the stored choices
    void init(UserSettingsPointer pConfig);

    const QList<ControllerLayoutControl>& controls() const {
        return m_controls;
    }
    QString imagePath() const {
        return m_imagePath;
    }
    /// The image's size in layout units
    double imageWidth() const {
        return m_imageWidth;
    }
    double imageHeight() const {
        return m_imageHeight;
    }

    /// "[Group],item", or empty for the controller mapping
    QString target(const QString& inputKey, bool shift) const;
    void setTarget(const QString& inputKey, bool shift, const QString& configKey);
    void clear(const QString& inputKey, bool shift);
    void clearAll();
    int count() const;

    /// Called for every incoming MIDI message, before the mapping. Returns
    /// true when an override handled it (the mapping is then skipped).
    bool handleMidi(unsigned char status, unsigned char data1, unsigned char value);

  private:
    struct Route {
        QString inputKey;
        QString target; // "[Group],item"
        ControllerInput::Kind kind;
        bool lsb = false;
        double step = 0.01; // relative: parameter change per tick
    };

    ControllerOverrides() = default;
    void loadLayout(const QString& path);
    void load();
    void save() const;
    void rebuildRoutes();
    void apply(const Route& route, int value);

    UserSettingsPointer m_pConfig;
    QList<ControllerLayoutControl> m_controls;
    QString m_imagePath;
    double m_imageWidth = 0;
    double m_imageHeight = 0;

    mutable QMutex m_mutex;
    QHash<QString, QString> m_targets; // "<input key>[@shift]" -> "[Group],item"
    QHash<int, Route> m_routes;        // (status << 8 | data1) -> route
    QHash<QString, int> m_msb;         // last MSB of a 14-bit input
    QHash<QString, bool> m_pressed;    // a fader driving a button: last state
};

} // namespace djmantra
