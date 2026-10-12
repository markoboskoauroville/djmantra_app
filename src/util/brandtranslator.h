#pragma once

#include <QTranslator>

namespace djmantra {

/// DJ Mantra's name in every text the interface shows (Marko, 12.10.2026:
/// "Do you want to quit Mixxx?" on the phone). Installed after the other
/// translators, it is asked first: for a text that names Mixxx it takes the
/// translation from the others (or the English text) and puts in DJ Mantra.
/// Other texts it leaves to the others. Code names, file paths, settings
/// keys and the copyright and licence notices are not texts it sees.
class BrandTranslator : public QTranslator {
    Q_OBJECT
  public:
    explicit BrandTranslator(QObject* pParent);

    QString translate(const char* context,
            const char* sourceText,
            const char* disambiguation = nullptr,
            int n = -1) const override;
    bool isEmpty() const override {
        return false;
    }

    /// "Quit Mixxx?" -> "Quit DJ Mantra?"
    static QString rebrand(QString text);
};

} // namespace djmantra
