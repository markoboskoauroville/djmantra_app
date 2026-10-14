#include "util/brandtranslator.h"

#include <gtest/gtest.h>

using djmantra::BrandTranslator;

TEST(BrandTranslatorTest, rebrandsTheName) {
    EXPECT_EQ(QStringLiteral("Discard any changes and exit DJ Mantra?"),
            BrandTranslator::rebrand(QStringLiteral("Discard any changes and exit Mixxx?")));
    EXPECT_EQ(QStringLiteral("DJ Mantra's library"),
            BrandTranslator::rebrand(QStringLiteral("Mixxx's library")));
    EXPECT_EQ(QStringLiteral("WELCOME TO DJ MANTRA"),
            BrandTranslator::rebrand(QStringLiteral("WELCOME TO MIXXX")));
    // Not links or control names
    EXPECT_EQ(QStringLiteral("https://mixxx.org and MixxxControl"),
            BrandTranslator::rebrand(QStringLiteral("https://mixxx.org and MixxxControl")));
}

TEST(BrandTranslatorTest, answersOnlyForTextsWithTheName) {
    BrandTranslator translator(nullptr);
    EXPECT_TRUE(translator.translate("Ctx", "Play").isNull());
    EXPECT_EQ(QStringLiteral("Quit DJ Mantra"), translator.translate("Ctx", "Quit Mixxx"));
}
