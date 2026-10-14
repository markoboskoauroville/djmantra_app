#include "util/brandtranslator.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include "moc_brandtranslator.cpp"

namespace djmantra {

BrandTranslator::BrandTranslator(QObject* pParent)
        : QTranslator(pParent) {
}

// static
QString BrandTranslator::rebrand(QString text) {
    // The word Mixxx (also in "Mixxx's", "MIXXX"), not mixxxdj.org or a
    // control name such as "MixxxControl"
    static const QRegularExpression kWord(QStringLiteral("\\bMixxx\\b(?!\\.org|dj)"));
    static const QRegularExpression kUpper(QStringLiteral("\\bMIXXX\\b"));
    text.replace(kWord, QStringLiteral("DJ Mantra"));
    text.replace(kUpper, QStringLiteral("DJ MANTRA"));
    return text;
}

QString BrandTranslator::translate(const char* context,
        const char* sourceText,
        const char* disambiguation,
        int n) const {
    if (!sourceText) {
        return QString();
    }
    const QString source = QString::fromUtf8(sourceText);
    if (!source.contains(QLatin1String("Mixxx"), Qt::CaseInsensitive)) {
        // Not ours: the other translators answer
        return QString();
    }
    // The translation from the other translators (the last installed first,
    // as Qt asks them), else the English text
    QString text;
    const auto translators = QCoreApplication::instance()->findChildren<QTranslator*>();
    for (auto it = translators.crbegin(); it != translators.crend() && text.isEmpty(); ++it) {
        if (*it != this) {
            text = (*it)->translate(context, sourceText, disambiguation, n);
        }
    }
    if (text.isEmpty()) {
        // (%n is filled in by QCoreApplication::translate() afterwards)
        text = source;
    }
    return rebrand(text);
}

} // namespace djmantra
