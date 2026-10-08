#pragma once

#include <QString>

namespace mixxx {

/// DJ Mantra on Android: the skins, controller mappings and other resources
/// are packaged in the APK (assets:/res, see
/// tools/android/stage_android_package.cmake) and copied to app storage on
/// the first start of every new build, because the rest of the app reads them
/// as ordinary files.
///
/// `sourceDir` holds the files, an index (kIndexFile: one relative path per
/// line) and a stamp (kStampFile) that changes with every build. If
/// `targetDir` already has the same stamp nothing is copied. Returns
/// `targetDir`, or an empty string if the bundle has no stamp or index.
class BundledResources {
  public:
    static const QString kIndexFile;
    static const QString kStampFile;

    static QString install(const QString& sourceDir, const QString& targetDir);
};

} // namespace mixxx
