#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQmlParserStatus>
#include <QTimer>
#include <memory>

#include "control/controlproxy.h"

namespace mixxx {
namespace qml {

class QmlControlProxy : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    // TODO: The REQUIRED flag only exists in Qt 5.14 and later. Once we
    // require that as minimum dependency, add it to the group and key
    // properties.
    Q_PROPERTY(QString group READ getGroup WRITE setGroup NOTIFY groupChanged REQUIRED)
    Q_PROPERTY(QString key READ getKey WRITE setKey NOTIFY keyChanged REQUIRED)
    Q_PROPERTY(bool keyValid READ isKeyValid NOTIFY keyValidChanged)
    Q_PROPERTY(bool initialized READ isInitialized NOTIFY initializedChanged)
    Q_PROPERTY(double value READ getValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double parameter READ getParameter WRITE setParameter NOTIFY parameterChanged)
    /// DJ Mantra: at most this many value changes a second reach QML (0: all).
    /// The play position changes with every audio buffer, hundreds of times a
    /// second; a time display needs a few (round 24: the main thread ~24 %)
    Q_PROPERTY(int maxRate READ maxRate WRITE setMaxRate NOTIFY maxRateChanged)
    QML_NAMED_ELEMENT(ControlProxy)

  public:
    explicit QmlControlProxy(QObject* parent = nullptr);

    /// Implementing the QQmlParserStatus interface requires overriding this
    /// method, but we don't need it.
    // Invoked after class creation, but before any properties have been set.
    void classBegin() override{};

    /// QML cannot pass arguments to C++ constructors so this class needs to
    /// rely on the QML object setting the group and key properties to
    /// initialize the ControlProxy. We want to deplay the initialization of
    /// the underlying ControlProxy until the object has been fully created and
    /// all properties (group and key in particular) have been set.  Perform
    /// some initialization here now that the object is fully created.
    void componentComplete() override;

    void setGroup(const QString& group);
    const QString& getGroup() const;

    void setKey(const QString& key);
    const QString& getKey() const;

    void setValue(double newValue);
    double getValue() const;

    void setParameter(double newValue);
    double getParameter() const;

    int maxRate() const {
        return m_maxRate;
    }
    void setMaxRate(int rate);

    bool isKeyValid() const;
    bool isInitialized() const;

    /// Reset the control to the default value.
    Q_INVOKABLE void reset();

  signals:
    void groupChanged(const QString& group);
    void keyChanged(const QString& key);
    void keyValidChanged(bool valid);
    void initializedChanged(bool initialized);
    void valueChanged(double newValue);
    void parameterChanged(double newParameter);
    void maxRateChanged(int rate);

  private slots:
    /// Emits both the valueChanged and parameterChanged signals
    void slotControlProxyValueChanged(double newValue);
    /// The engine's change, through the maxRate limit
    void slotThrottledValueChanged(double newValue);

  private:
    /// (Re-)Initializes or resets the pointer to the underlying control proxy.
    /// Called for the first time when component construction has been
    /// completed. From that moment on, it's called whenever the group or key
    /// changes.
    void reinitializeFromKey();

    ConfigKey m_coKey;

    /// Set to true in the componentComplete() method, which is called when the
    /// QML object creation is complete.
    bool m_isComponentComplete;
    std::unique_ptr<ControlProxy> m_pControlProxy;

    int m_maxRate = 0;
    QTimer m_rateTimer;
    bool m_changedMeanwhile = false;
};

} // namespace qml
} // namespace mixxx
