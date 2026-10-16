#include "util/savedfiles.h"

#include <QFileInfo>
#include <QtDebug>

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniObject>
#endif

namespace djmantra {

void announceSaved(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
#if defined(Q_OS_ANDROID)
    // the toast and the log line come from Java (one place for both sides)
    QJniObject::callStaticMethod<void>("com/djmantra/app/SavedFiles",
            "announce",
            "(Landroid/content/Context;Ljava/lang/String;)V",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(QFileInfo(path).absoluteFilePath()).object<jstring>());
#else
    qInfo() << "Saved file:" << QFileInfo(path).absoluteFilePath();
#endif
}

} // namespace djmantra
