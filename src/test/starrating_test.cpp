// DJ Mantra: compact star rating in track tables ("★ 3", click to raise).

#include "library/starrating.h"

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

TEST(StarRatingTest, clickCyclesThroughAllRatings) {
    constexpr int kMax = 5;
    int rating = 0;
    const int expected[] = {1, 2, 3, 4, 5, 0, 1};
    for (int next : expected) {
        rating = StarRating::nextStarCountOnClick(rating, kMax);
        EXPECT_EQ(next, rating);
    }
}

TEST(StarRatingTest, clickRecoversFromOutOfRangeValues) {
    EXPECT_EQ(0, StarRating::nextStarCountOnClick(-1, 5));
    EXPECT_EQ(0, StarRating::nextStarCountOnClick(7, 5));
}

TEST(StarRatingTest, compactFormIsNarrowerThanFiveStars) {
    const StarRating rating(3);
    EXPECT_LT(rating.compactSizeHint().width(), rating.sizeHint().width());
    EXPECT_EQ(rating.sizeHint().height(), rating.compactSizeHint().height());
}

TEST(StarRatingTest, compactFormPaintsStarAndNumber) {
    // Rated: a star and a digit are drawn, so more pixels are covered than
    // for the unrated diamond.
    auto inkedPixels = [](int stars) {
        QImage image(60, 20, QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setBrush(Qt::black);
        StarRating(stars).paintCompact(&painter, image.rect());
        painter.end();
        int inked = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (qGray(image.pixel(x, y)) < 128) {
                    ++inked;
                }
            }
        }
        return inked;
    };
    const int unrated = inkedPixels(0);
    const int rated = inkedPixels(3);
    EXPECT_GT(unrated, 0);
    EXPECT_GT(rated, unrated * 3);
}
