#pragma once

#include <QString>
#include <functional>
#include <memory>

#include "sources/externalcontent.h"

namespace djmantra {

/// Android: drives through the Storage Access Framework
/// (packaging/android/package/src/com/djmantra/app/StorageBridge.java).
class AndroidStorage {
  public:
    /// The provider for ExternalContent (nullptr on other systems).
    static std::shared_ptr<ExternalContentProvider> createProvider();

    /// Opens the system folder picker (on the drive `volumeId` if given). The
    /// callback gets the chosen folder as a path the library can use: a real
    /// path on internal storage, or the placeholder directory of a drive
    /// (after its placeholders were created); empty if cancelled.
    static void pickFolder(const QString& volumeId, std::function<void(const QString&)> callback);
};

} // namespace djmantra
