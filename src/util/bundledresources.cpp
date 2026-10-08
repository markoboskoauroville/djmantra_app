#include "util/bundledresources.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtDebug>

namespace mixxx {

const QString BundledResources::kIndexFile = QStringLiteral("djmantra-res.index");
const QString BundledResources::kStampFile = QStringLiteral("djmantra-res.stamp");

namespace {

QByteArray readAll(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

} // namespace

// static
QString BundledResources::install(const QString& sourceDir, const QString& targetDir) {
    const QByteArray stamp = readAll(sourceDir + QChar('/') + kStampFile).trimmed();
    if (stamp.isEmpty()) {
        qWarning() << "No bundled resources in" << sourceDir;
        return QString();
    }
    if (readAll(targetDir + QChar('/') + kStampFile).trimmed() == stamp) {
        return targetDir;
    }
    const QByteArray index = readAll(sourceDir + QChar('/') + kIndexFile);
    if (index.isEmpty()) {
        qWarning() << "No resource index in" << sourceDir;
        return QString();
    }

    qInfo() << "Installing resources from" << sourceDir << "to" << targetDir;
    // Files of an older build must not stay behind
    QDir(targetDir).removeRecursively();
    QDir target(targetDir);
    target.mkpath(QStringLiteral("."));
    int copied = 0;
    int failed = 0;
    const QList<QByteArray> lines = index.split('\n');
    for (const QByteArray& line : lines) {
        const QString relativePath = QString::fromUtf8(line).trimmed();
        if (relativePath.isEmpty() || relativePath.startsWith(QStringLiteral(".."))) {
            continue;
        }
        const QString to = target.filePath(relativePath);
        target.mkpath(QFileInfo(relativePath).path());
        if (QFile::copy(sourceDir + QChar('/') + relativePath, to)) {
            // Assets are read-only; the copies need not be
            QFile::setPermissions(to,
                    QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser |
                            QFile::WriteUser);
            ++copied;
        } else {
            ++failed;
            qWarning() << "Could not copy resource" << relativePath;
        }
    }
    qInfo() << "Installed" << copied << "resource files," << failed << "failed";

    // Written last: an interrupted copy is repeated on the next start
    QFile stampFile(target.filePath(kStampFile));
    if (stampFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        stampFile.write(stamp);
    }
    return targetDir;
}

} // namespace mixxx
