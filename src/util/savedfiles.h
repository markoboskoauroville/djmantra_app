#pragma once

#include <QString>

namespace djmantra {

/// DJ Mantra (Marko 16.10.2026: "any file type is saved ... please post where the file is,
/// the location and name"): says where a file the app just saved is. On Android a long toast
/// with the name and the folder (com.djmantra.app.SavedFiles); everywhere a log line
/// "Saved file: <path>".
void announceSaved(const QString& path);

} // namespace djmantra
