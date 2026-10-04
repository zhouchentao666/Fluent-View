#pragma once

#include <QString>

class QWidget;

namespace fv {

namespace PlatformIntegration {

// Registers the app as the default viewer for common image types.
// Windows: writes HKCU\Software\Classes (no admin needed) + offers the
//          modern "Default apps" page for per-type confirmation.
// Linux:   writes a .desktop file with MimeType list into ~/.local/share.
// macOS:   registration is Info.plist-driven; opens System Settings hint.
void registerAsDefaultViewer(QWidget *parent = nullptr);

// Sets the desktop wallpaper (Windows: SystemParametersInfo, macOS: NSWorkspace
// via osascript, Linux: falls back to pcmanfm/feh if present).
bool setWallpaper(const QString &imagePath);

// Opens the parent folder with the file pre-selected.
void revealInFileManager(const QString &path);

} // namespace PlatformIntegration
} // namespace fv
