#include "sources/androidstorage.h"

#include "util/logger.h"

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QDir>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStandardPaths>
#include <QStringList>
#include <QtCore/private/qandroidextras_p.h>
#endif

namespace djmantra {

namespace {

const mixxx::Logger kLogger("AndroidStorage");

#if defined(Q_OS_ANDROID)

constexpr const char* kBridge = "com/djmantra/app/StorageBridge";
constexpr int kPickFolderRequest = 0x444a; // "DJ"

QJniObject context() {
    return QJniObject(QNativeInterface::QAndroidApplication::context());
}

QStringList toStringList(const QJniObject& array) {
    QStringList list;
    if (!array.isValid()) {
        return list;
    }
    QJniEnvironment env;
    const auto jArray = static_cast<jobjectArray>(array.object());
    const jsize size = env->GetArrayLength(jArray);
    for (jsize i = 0; i < size; ++i) {
        const QJniObject element = QJniObject::fromLocalRef(env->GetObjectArrayElement(jArray, i));
        list.append(element.toString());
    }
    return list;
}

class SafProvider final : public ExternalContentProvider {
  public:
    QList<Volume> volumes() override {
        QList<Volume> result;
        const QJniObject array = QJniObject::callStaticObjectMethod(kBridge,
                "volumes",
                "(Landroid/content/Context;)[Ljava/lang/String;",
                context().object());
        const QStringList lines = toStringList(array);
        for (const auto& line : lines) {
            const QStringList fields = line.split(QChar('\t'));
            if (fields.size() < 4) {
                continue;
            }
            Volume volume;
            volume.id = fields[0];
            volume.label = fields[1];
            volume.mounted = fields[2] == QStringLiteral("1");
            volume.granted = fields[3] == QStringLiteral("1");
            result.append(volume);
        }
        return result;
    }

    bool list(const QString& volumeId, QList<Entry>* pEntries) override {
        const QJniObject array = QJniObject::callStaticObjectMethod(kBridge,
                "list",
                "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
                context().object(),
                QJniObject::fromString(volumeId).object<jstring>());
        if (!array.isValid()) {
            return false;
        }
        const QStringList lines = toStringList(array);
        for (const auto& line : lines) {
            const QStringList fields = line.split(QChar('\t'));
            if (fields.size() < 3) {
                continue;
            }
            pEntries->append(Entry{fields[0], fields[1].toLongLong(), fields[2].toLongLong()});
        }
        return true;
    }

    int openForReading(const QString& volumeId, const QString& relativePath) override {
        return QJniObject::callStaticMethod<jint>(kBridge,
                "openForReading",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
                context().object(),
                QJniObject::fromString(volumeId).object<jstring>(),
                QJniObject::fromString(relativePath).object<jstring>());
    }
};

#endif

} // namespace

std::shared_ptr<ExternalContentProvider> AndroidStorage::createProvider() {
#if defined(Q_OS_ANDROID)
    return std::make_shared<SafProvider>();
#else
    return nullptr;
#endif
}

void AndroidStorage::pickFolder(
        const QString& volumeId, std::function<void(const QString&)> callback) {
#if defined(Q_OS_ANDROID)
    const QJniObject intent = QJniObject::callStaticObjectMethod(kBridge,
            "treePickerIntent",
            "(Landroid/content/Context;Ljava/lang/String;)Landroid/content/Intent;",
            context().object(),
            QJniObject::fromString(volumeId).object<jstring>());
    // The result comes on Android's thread: hand it to the Qt GUI thread,
    // where the library and dialogs live.
    const auto deliver = [callback](const QString& folder) {
        QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [callback, folder] { callback(folder); },
                Qt::QueuedConnection);
    };
    QtAndroidPrivate::startActivity(intent,
            kPickFolderRequest,
            [callback = deliver](int requestCode, int resultCode, const QJniObject& data) {
                constexpr int kResultOk = -1; // Activity.RESULT_OK
                if (requestCode != kPickFolderRequest || resultCode != kResultOk) {
                    callback(QString());
                    return;
                }
                const QString granted = QJniObject::callStaticObjectMethod(kBridge,
                        "persistTree",
                        "(Landroid/content/Context;Landroid/content/Intent;)Ljava/lang/String;",
                        context().object(),
                        data.object())
                                                .toString();
                const qsizetype tab = granted.indexOf(QChar('\t'));
                if (tab <= 0) {
                    callback(QString());
                    return;
                }
                const QString volume = granted.left(tab);
                const QString path = granted.mid(tab + 1);
                kLogger.info() << "Folder granted on" << volume << ":" << path;
                if (volume == QStringLiteral("primary")) {
                    // Internal storage: readable by path ("All files access")
                    callback(QDir::cleanPath(
                            QStringLiteral("/storage/emulated/0/") + path));
                    return;
                }
                callback(QDir::cleanPath(
                        ExternalContent::volumeDirectory(volume) + QChar('/') + path));
            });
#else
    Q_UNUSED(volumeId);
    callback(QString());
#endif
}

} // namespace djmantra
