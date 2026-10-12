#include "widget/djmantra/camelot.h"

#include <gtest/gtest.h>

#include "track/keyutils.h"

using namespace djmantra::camelot;
namespace key = mixxx::track::io::key;

TEST(CamelotTest, keysAsCamelotCodes) {
    EXPECT_EQ(QStringLiteral("8A"), text(fromKey(key::A_MINOR)));
    EXPECT_EQ(QStringLiteral("8B"), text(fromKey(key::C_MAJOR)));
    EXPECT_EQ(QStringLiteral("5A"), text(fromKey(key::C_MINOR)));
    EXPECT_EQ(QStringLiteral("12B"), text(fromKey(key::E_MAJOR)));
    EXPECT_EQ(QStringLiteral("-"), text(fromKey(key::INVALID)));
    // And back, for all 24
    for (int value = key::C_MAJOR; value <= key::B_MINOR; ++value) {
        const auto chromatic = static_cast<key::ChromaticKey>(value);
        EXPECT_EQ(chromatic, toKey(fromKey(chromatic))) << value;
        // The same as Mixxx's Lancelot notation
        EXPECT_EQ(KeyUtils::keyToString(chromatic, KeyUtils::KeyNotation::Lancelot),
                text(fromKey(chromatic)));
    }
}

TEST(CamelotTest, howKeysMatch) {
    const Code a8{8, false};
    EXPECT_EQ(Match::Same, match(a8, Code{8, false}));
    EXPECT_EQ(Match::Compatible, match(a8, Code{9, false}));
    EXPECT_EQ(Match::Compatible, match(a8, Code{7, false}));
    EXPECT_EQ(Match::Compatible, match(a8, Code{8, true}));
    EXPECT_EQ(Match::Risky, match(a8, Code{10, false}));
    EXPECT_EQ(Match::Risky, match(a8, Code{3, false})); // +7
    EXPECT_EQ(Match::Risky, match(a8, Code{9, true}));  // diagonal
    EXPECT_EQ(Match::Clash, match(a8, Code{2, false}));
    EXPECT_EQ(Match::Clash, match(a8, Code{11, true}));
    // Round the top of the wheel: 12 > 1 is +1
    EXPECT_EQ(Match::Compatible, match(Code{12, true}, Code{1, true}));
    EXPECT_EQ(Match::None, match(Code{}, a8));
    QString why;
    match(a8, Code{9, false}, &why);
    EXPECT_FALSE(why.isEmpty());
}

TEST(CamelotTest, keyMatchShift) {
    const auto code = [](int number, bool major) {
        Code c;
        c.number = number;
        c.major = major;
        return c;
    };
    // Round 16b: E major (12B) against A minor (8A) goes to C major (8B), -4
    EXPECT_EQ(-4, matchShift(code(12, true), code(8, false)));
    EXPECT_EQ(0, matchShift(code(8, false), code(8, false)));
    EXPECT_EQ(0, matchShift(code(8, true), code(8, false)));
    // One step on the wheel is a fifth: +7 = -5 semitones
    EXPECT_EQ(-5, matchShift(code(8, false), code(9, false)));
    EXPECT_EQ(5, matchShift(code(9, false), code(8, false)));
    EXPECT_EQ(0, matchShift(code(0, false), code(8, false)));
    // Every pair lands on the other's number on the deck's own ring
    for (int from = 1; from <= 12; ++from) {
        for (int to = 1; to <= 12; ++to) {
            for (const bool major : {false, true}) {
                const auto deck = code(from, major);
                const int shift = matchShift(deck, code(to, !major));
                EXPECT_LE(-5, shift);
                EXPECT_GE(6, shift);
                const auto chromatic = toKey(deck);
                const auto shifted = KeyUtils::scaleKeySteps(chromatic, shift);
                EXPECT_EQ(text(code(to, major)), text(fromKey(shifted)))
                        << text(deck).toStdString() << " > " << to;
            }
        }
    }
}
