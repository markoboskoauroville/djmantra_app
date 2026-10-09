#include "util/uitestprobe.h"

#include <QEvent>
#include <QScreen>
#include <QWidget>
#include <QWindow>
#include <QtDebug>

#include "control/controlproxy.h"
#include "moc_uitestprobe.cpp"
#include "widget/controlwidgetconnection.h"
#include "widget/wbasewidget.h"
#include "widget/wwaveformviewer.h"

namespace djmantra {

namespace {

// Controls that change all the time: not logged on every change
bool isNoisy(const QString& item) {
    return item == QLatin1String("playposition") || item == QLatin1String("vu_meter") ||
            item.startsWith(QLatin1String("vu_meter")) ||
            item == QLatin1String("visual_bpm") || item == QLatin1String("visual_key") ||
            item == QLatin1String("time_remaining") || item == QLatin1String("time_elapsed") ||
            item.startsWith(QLatin1String("peak_indicator"));
}

QString widgetType(const QWidget* pWidget) {
    QString type = QString::fromLatin1(pWidget->metaObject()->className());
    if (type.startsWith(QLatin1Char('W'))) {
        type.remove(0, 1);
    }
    return type;
}

} // namespace

UiTestProbe::UiTestProbe(QWidget* pWindow)
        : QObject(pWindow),
          m_pWindow(pWindow),
          m_mapNumber(0) {
    m_dumpTimer.setSingleShot(true);
    m_dumpTimer.setInterval(800);
    connect(&m_dumpTimer, &QTimer::timeout, this, &UiTestProbe::dumpWidgetMap);
    pWindow->installEventFilter(this);
    // The skin's pages: a new map after every switch
    for (const auto& item : {"show_library",
                 "waveform_fullscreen",
                 "l_mixer",
                 "l_waveforms",
                 "l_pads",
                 "p_mixer",
                 "p_waveforms",
                 "p_pads",
                 "p_deck1",
                 "p_deck2"}) {
        auto* pProxy = new ControlProxy(QStringLiteral("[DJMantra]"),
                QString::fromLatin1(item),
                this,
                ControlFlag::NoWarnIfMissing);
        if (pProxy->valid()) {
            pProxy->connectValueChanged(this, [this] { scheduleDump(); });
        }
    }
    // The VU meters' input, once a second (round 4: the meters on the
    // emulator stayed grey while sound played)
    auto* pLevels = new QTimer(this);
    QList<ControlProxy*> levels;
    for (const auto& group : {"[Channel1]", "[Channel2]", "[Main]"}) {
        levels.append(new ControlProxy(QString::fromLatin1(group),
                QStringLiteral("vu_meter"),
                this,
                ControlFlag::NoWarnIfMissing));
    }
    connect(pLevels, &QTimer::timeout, this, [levels] {
        QStringList values;
        for (auto* pLevel : levels) {
            values.append(QString::number(pLevel->valid() ? pLevel->get() : -1, 'f', 3));
        }
        qInfo().noquote() << "UI levels" << values.join(QLatin1Char(' '));
    });
    pLevels->start(1000);
    qInfo() << "UI test probe on";
    scheduleDump();
}

bool UiTestProbe::eventFilter(QObject* pObject, QEvent* pEvent) {
    if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
        scheduleDump();
    }
    return QObject::eventFilter(pObject, pEvent);
}

void UiTestProbe::scheduleDump() {
    m_dumpTimer.start();
}

void UiTestProbe::watchControl(const QString& group, const QString& item) {
    const QString key = group + QLatin1Char(',') + item;
    if (m_watched.contains(key) || isNoisy(item)) {
        return;
    }
    m_watched.insert(key);
    auto* pProxy = new ControlProxy(group, item, this, ControlFlag::NoWarnIfMissing);
    if (!pProxy->valid()) {
        return;
    }
    m_proxies.append(pProxy);
    pProxy->connectValueChanged(this, [key](double value) {
        qInfo().noquote() << "UI control" << key << "=" << value;
    });
}

void UiTestProbe::dumpWidgetMap() {
    if (!m_pWindow) {
        return;
    }
    ++m_mapNumber;
    const qreal ratio = m_pWindow->devicePixelRatioF();
    const QSize size = m_pWindow->size() * ratio;
    qInfo().noquote() << "UI map begin" << m_mapNumber << size.width() << size.height()
                      << "ratio" << ratio;
    const auto widgets = m_pWindow->findChildren<QWidget*>();
    for (QWidget* pWidget : widgets) {
        if (!pWidget->isVisible() || pWidget->width() < 4 || pWidget->height() < 4) {
            continue;
        }
        QString key;
        auto* pBase = dynamic_cast<WBaseWidget*>(pWidget);
        if (pBase) {
            // A button's tap goes to its left-click connection; the others
            // (sliders, knobs, displays) have plain connections
            auto connections = pBase->leftConnections();
            connections.append(pBase->connections());
            if (!connections.isEmpty()) {
                const ConfigKey& configKey = connections.first()->getKey();
                key = configKey.group + QLatin1Char(',') + configKey.item;
                for (const auto* pConnection : std::as_const(connections)) {
                    watchControl(pConnection->getKey().group, pConnection->getKey().item);
                }
            }
        }
        if (auto* pViewer = qobject_cast<WWaveformViewer*>(pWidget)) {
            key = pViewer->getGroup() + QStringLiteral(",waveform");
        }
        if (key.isEmpty()) {
            continue;
        }
        const QPoint topLeft = pWidget->mapTo(m_pWindow, QPoint(0, 0));
        const QRect rect(QPoint(qRound(topLeft.x() * ratio), qRound(topLeft.y() * ratio)),
                pWidget->size() * ratio);
        // Partly hidden behind its parents (a scrolled or clipped area)
        const QRect visible = pWidget->visibleRegion().boundingRect();
        qInfo().noquote() << "UI widget" << widgetType(pWidget)
                          << (pWidget->objectName().isEmpty() ? QStringLiteral("-")
                                                              : pWidget->objectName())
                          << key << rect.x() << rect.y() << rect.width() << rect.height()
                          << (visible.size() == pWidget->size() ? "full" : "clipped");
    }
    qInfo().noquote() << "UI map end" << m_mapNumber;
}

} // namespace djmantra
