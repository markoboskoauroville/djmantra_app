#include "util/bundledresources.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using mixxx::BundledResources;

namespace {

void writeFile(const QString& path, const QByteArray& content) {
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

class BundledResourcesTest : public testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_dir.isValid());
        m_source = m_dir.filePath(QStringLiteral("assets/res"));
        m_target = m_dir.filePath(QStringLiteral("files/res"));
        writeFile(m_source + "/skins/LateNight/skin.xml", "<skin/>");
        writeFile(m_source + "/controllers/Hercules Mix Ultra.midi.xml", "<map/>");
        writeFile(m_source + "/" + BundledResources::kIndexFile,
                "skins/LateNight/skin.xml\ncontrollers/Hercules Mix Ultra.midi.xml\n");
        writeFile(m_source + "/" + BundledResources::kStampFile, "build-1\n");
    }

    QTemporaryDir m_dir;
    QString m_source;
    QString m_target;
};

TEST_F(BundledResourcesTest, CopiesEveryIndexedFile) {
    EXPECT_EQ(m_target, BundledResources::install(m_source, m_target));
    EXPECT_EQ("<skin/>", readFile(m_target + "/skins/LateNight/skin.xml"));
    EXPECT_EQ("<map/>", readFile(m_target + "/controllers/Hercules Mix Ultra.midi.xml"));
    EXPECT_EQ("build-1", readFile(m_target + "/" + BundledResources::kStampFile));
}

TEST_F(BundledResourcesTest, SameStampCopiesNothing) {
    BundledResources::install(m_source, m_target);
    writeFile(m_target + "/skins/LateNight/skin.xml", "changed");
    EXPECT_EQ(m_target, BundledResources::install(m_source, m_target));
    EXPECT_EQ("changed", readFile(m_target + "/skins/LateNight/skin.xml"));
}

TEST_F(BundledResourcesTest, NewBuildReplacesOldFiles) {
    BundledResources::install(m_source, m_target);
    writeFile(m_target + "/skins/Old/skin.xml", "old");
    writeFile(m_source + "/" + BundledResources::kStampFile, "build-2\n");
    EXPECT_EQ(m_target, BundledResources::install(m_source, m_target));
    EXPECT_FALSE(QFile::exists(m_target + "/skins/Old/skin.xml"));
    EXPECT_EQ("<skin/>", readFile(m_target + "/skins/LateNight/skin.xml"));
    EXPECT_EQ("build-2", readFile(m_target + "/" + BundledResources::kStampFile));
}

TEST_F(BundledResourcesTest, NoBundleGivesEmptyPath) {
    EXPECT_TRUE(BundledResources::install(m_dir.filePath("missing"), m_target).isEmpty());
}

} // namespace
