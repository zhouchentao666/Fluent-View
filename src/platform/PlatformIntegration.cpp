#include "platform/PlatformIntegration.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shellapi.h>
#  pragma comment(lib, "shell32.lib")
#  pragma comment(lib, "user32.lib")
#endif

namespace fv {
namespace PlatformIntegration {

namespace {

const char kProgId[] = "FluentView.Image";
const char kProgIdName[] = "Fluent-View Image";

QStringList commonImageSuffixes()
{
    return {
        "jpg", "jpeg", "jpe", "jfif", "png", "apng", "bmp", "dib", "gif",
        "webp", "tif", "tiff", "ico", "heic", "heif", "avif"
    };
}

#ifdef Q_OS_WIN
QStringList installedSuffixes()
{
    // Only register types whose registry extension exists (heuristic);
    // simple approach: always register the common set under HKCU.
    return commonImageSuffixes();
}

void writeProgId()
{
    QSettings progId(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\") + kProgId,
                     QSettings::NativeFormat);
    progId.setValue(".", kProgIdName);
    progId.setValue("DefaultIcon/.", QString("\"%1\",0").arg(
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath())));
    progId.setValue("shell/open/command/.",
                    QString("\"%1\" \"%2\"").arg(
                        QDir::toNativeSeparators(QCoreApplication::applicationFilePath()),
                        "%1"));
    progId.setValue("shell/open/DdeCommand.", "");
    progId.sync();
}
#endif

} // namespace

#ifdef Q_OS_WIN
void registerAsDefaultViewer(QWidget *parent)
{
    writeProgId();

    // Register extensions + tell the shell to refresh icon cache.
    for (const QString &ext : installedSuffixes()) {
        QSettings extKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\.%1").arg(ext),
                         QSettings::NativeFormat);
        extKey.setValue("OpenWithProgids", QStringList{kProgId});
        extKey.sync();
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    // The user must confirm via the modern "Default apps" page on Win10/11.
    const int choice = QMessageBox::question(
        parent,
        QObject::tr("Set as default viewer"),
        QObject::tr("Fluent-View was registered for common image types.\n\n"
                    "Windows 10/11 requires confirming the default app per type.\n"
                    "Open 'Default apps' settings now?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (choice == QMessageBox::Yes) {
        QProcess::startDetached(QStringLiteral("ms-settings:defaultapps"),
                                QStringList());
    }
}
#else
void registerAsDefaultViewer(QWidget *parent)
{
#  ifdef Q_OS_LINUX
    // Write a user-level desktop entry and register MIME types.
    const QString localApps =
        QDir::homePath() + QStringLiteral("/.local/share/applications");
    QDir().mkpath(localApps);
    const QString desktopFile = localApps + QStringLiteral("/fluent-view.desktop");
    QFile f(desktopFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(QString(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Fluent-View\n"
            "Exec=fluent-view %F\n"
            "TryExec=fluent-view\n"
            "Icon=com.fluentview.FluentView\n"
            "Terminal=false\n"
            "Categories=Graphics;Viewer;2DGraphics;\n"
            "MimeType=image/jpeg;image/png;image/bmp;image/gif;image/webp;"
            "image/tiff;image/x-icon;image/avif;image/heif;image/heic;\n"
            "NoDisplay=false\n").toUtf8());
        f.close();
    }
    QProcess::execute(QStringLiteral("update-desktop-database"), { localApps });

    QMessageBox::information(
        parent, QObject::tr("Set as default viewer"),
        QObject::tr("A desktop entry was installed for this user.\n"
                    "Use your desktop environment's 'Default Applications' panel "
                    "to choose Fluent-View for image types."));
#  elif defined(Q_OS_MACOS)
    QMessageBox::information(
        parent, QObject::tr("Set as default viewer"),
        QObject::tr("On macOS, file associations come from the app bundle's Info.plist.\n"
                    "Right-click an image in Finder > Get Info > 'Open with' > "
                    "'Change All...' to make Fluent-View the default."));
#  endif
    Q_UNUSED(parent);
}
#endif

#ifdef Q_OS_WIN
bool setWallpaper(const QString &imagePath)
{
    const QString native = QDir::toNativeSeparators(QFileInfo(imagePath).absoluteFilePath());
    return SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                                 (void *) native.utf16(),
                                 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != FALSE;
}
#elif defined(Q_OS_MACOS)
bool setWallpaper(const QString &imagePath)
{
    QProcess osascript;
    osascript.start(QStringLiteral("osascript"));
    osascript.waitForStarted(1000);
    osascript.write(QString(
        "tell application \"System Events\" to tell every desktop to set picture to \"%1\"\n")
        .arg(QDir::toNativeSeparators(QFileInfo(imagePath).absoluteFilePath())).toUtf8());
    osascript.closeWriteChannel();
    osascript.waitForFinished(3000);
    return osascript.exitCode() == 0;
}
#else
bool setWallpaper(const QString &imagePath)
{
    // Best effort: common lightweight DE tools.
    for (const QString &cmd : { QStringLiteral("pcmanfm"), QStringLiteral("feh") }) {
        if (QStandardPaths::findExecutable(cmd).isEmpty())
            continue;
        if (cmd == QLatin1String("pcmanfm"))
            return QProcess::startDetached(cmd,
                { QStringLiteral("--set-wallpaper=") + QFileInfo(imagePath).absoluteFilePath() });
        return QProcess::startDetached(cmd,
                { QStringLiteral("--bg-scale"), QFileInfo(imagePath).absoluteFilePath() });
    }
    return false;
}
#endif

void revealInFileManager(const QString &path)
{
    const QFileInfo fi(path);
#  ifdef Q_OS_WIN
    QProcess::startDetached(QStringLiteral("explorer.exe"),
        { QStringLiteral("/select,"), QDir::toNativeSeparators(fi.absoluteFilePath()) });
#  elif defined(Q_OS_MACOS)
    QProcess::execute(QStringLiteral("open"), { QStringLiteral("-R"), fi.absoluteFilePath() });
#  else
    QProcess::startDetached(QStringLiteral("xdg-open"), { fi.absolutePath() });
#  endif
}

} // namespace PlatformIntegration
} // namespace fv
