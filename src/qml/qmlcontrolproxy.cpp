#include "qml/qmlcontrolproxy.h"

#include <algorithm>

#include "moc_qmlcontrolproxy.cpp"
#include "util/cmdlineargs.h"

namespace mixxx {
namespace qml {

namespace {
constexpr double kDefaultValue = 0.0;
constexpr double kDefaultParameter = 0.0;
} // namespace

QmlControlProxy::QmlControlProxy(QObject* parent)
        : QObject(parent),
          m_isComponentComplete(false),
          m_pControlProxy(nullptr) {
    m_rateTimer.setSingleShot(true);
    connect(&m_rateTimer, &QTimer::timeout, this, [this] {
        if (m_changedMeanwhile && isInitialized()) {
            m_changedMeanwhile = false;
            slotControlProxyValueChanged(m_pControlProxy->get());
            m_rateTimer.start();
        }
    });
}

void QmlControlProxy::setMaxRate(int rate) {
    rate = std::max(0, rate);
    if (rate == m_maxRate) {
        return;
    }
    m_maxRate = rate;
    if (rate > 0) {
        m_rateTimer.setInterval(std::max(1, 1000 / rate));
    } else {
        m_rateTimer.stop();
    }
    emit maxRateChanged(rate);
}

void QmlControlProxy::slotThrottledValueChanged(double newValue) {
    if (m_maxRate <= 0) {
        slotControlProxyValueChanged(newValue);
        return;
    }
    if (m_rateTimer.isActive()) {
        m_changedMeanwhile = true; // the newest value goes out when it ends
        return;
    }
    slotControlProxyValueChanged(newValue);
    m_rateTimer.start();
}

void QmlControlProxy::componentComplete() {
    m_isComponentComplete = true;
    reinitializeFromKey();
}

bool QmlControlProxy::isKeyValid() const {
    return m_coKey.isValid();
}

bool QmlControlProxy::isInitialized() const {
    return m_pControlProxy != nullptr;
}

void QmlControlProxy::setGroup(const QString& group) {
    if (m_coKey.group == group) {
        return;
    }

    const bool keyValidBeforeSet = isKeyValid();
    m_coKey.group = group;
    emit groupChanged(group);

    const bool keyValidAfterSet = isKeyValid();
    if (keyValidBeforeSet != keyValidAfterSet) {
        emit keyValidChanged(keyValidAfterSet);
    }

    reinitializeFromKey();
}

const QString& QmlControlProxy::getGroup() const {
    return m_coKey.group;
}

void QmlControlProxy::setKey(const QString& key) {
    if (m_coKey.item == key) {
        return;
    }

    const bool keyValidBeforeSet = isKeyValid();
    m_coKey.item = key;
    emit keyChanged(key);

    const bool keyValidAfterSet = isKeyValid();
    if (keyValidBeforeSet != keyValidAfterSet) {
        emit keyValidChanged(keyValidAfterSet);
    }

    reinitializeFromKey();
}

const QString& QmlControlProxy::getKey() const {
    return m_coKey.item;
}

void QmlControlProxy::setValue(double newValue) {
    if (!isInitialized()) {
        emit valueChanged(kDefaultValue);
        return;
    }
    m_pControlProxy->set(newValue);
    slotControlProxyValueChanged(newValue);
}

double QmlControlProxy::getValue() const {
    if (!isInitialized()) {
        return kDefaultValue;
    }
    return m_pControlProxy->get();
}

void QmlControlProxy::setParameter(double newValue) {
    if (!isInitialized()) {
        emit parameterChanged(kDefaultValue);
        return;
    }
    m_pControlProxy->setParameter(newValue);
    emit valueChanged(m_pControlProxy->get());
    emit parameterChanged(newValue);
}

double QmlControlProxy::getParameter() const {
    if (!isInitialized()) {
        return kDefaultParameter;
    }
    return m_pControlProxy->getParameter();
}

void QmlControlProxy::reset() {
    if (!isInitialized()) {
        return;
    }
    m_pControlProxy->reset();
}

void QmlControlProxy::reinitializeFromKey() {
    // Just ignore this if the component is still loading, because group or key may not be set yet.
    if (!m_isComponentComplete) {
        return;
    }

    // We don't need to reinitialize or reset the underlying control proxy if
    // the CO key didn't change.
    if (isInitialized() && m_coKey == m_pControlProxy->getKey()) {
        return;
    }

    // If the key is invalid, reset the control proxy if necessary.
    if (!isKeyValid()) {
        qWarning() << "QmlControlProxy: Tried to initialize CO" << m_coKey
                   << " with invalid key, resetting...";
        if (isInitialized()) {
            m_pControlProxy.reset();
            emit initializedChanged(false);
        }
        return;
    }

    // We don't need to warn here if the control is missing, because we'll do a
    // check below and print a warning anyway. If the key is invalid, we also don't
    // trigger an assert. This is to ensure backward compatibility with
    // ControlObjectScript::ControlObjectScript and let the caller handle a missing key
    std::unique_ptr<ControlProxy> pControlProxy =
            std::make_unique<ControlProxy>(m_coKey,
                    this,
                    CmdlineArgs::Instance().getDeveloper()
                            ? ControlFlag::NoWarnIfMissing
                            : ControlFlag::AllowMissingOrInvalid);

    // This should never happen, but it doesn't hurt to check.
    VERIFY_OR_DEBUG_ASSERT(pControlProxy != nullptr) {
        qWarning() << "QmlControlProxy: Requested CO " << m_coKey
                   << " returned nullptr, resetting...";
        if (isInitialized()) {
            m_pControlProxy.reset();
            emit initializedChanged(false);
        }
        return;
    }

    // If the control does not exist, reset the control proxy if necessary.
    if (!pControlProxy->valid()) {
        qWarning() << "QmlControlProxy: Requested CO" << m_coKey << " does not exist, resetting...";
        if (isInitialized()) {
            m_pControlProxy.reset();
            emit initializedChanged(false);
        }
        return;
    }

    // Set the control proxy and emit signal
    const bool wasInitialized = isInitialized();
    m_pControlProxy = std::move(pControlProxy);
    if (!wasInitialized) {
        emit initializedChanged(true);
    }
    m_pControlProxy->connectValueChanged(this, &QmlControlProxy::slotThrottledValueChanged);
    slotControlProxyValueChanged(m_pControlProxy->get());
}

void QmlControlProxy::slotControlProxyValueChanged(double newValue) {
    emit valueChanged(newValue);
    emit parameterChanged(m_pControlProxy->getParameter());
}

} // namespace qml
} // namespace mixxx
