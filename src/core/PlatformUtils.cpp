#include "PlatformUtils.h"
#include "Settings.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QImage>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>
#include <QProcess>
#include <QDir>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

PlatformUtils::PlatformUtils(QObject *parent) : QObject(parent) {}

bool PlatformUtils::isDesktop() const {
    const QString p = QGuiApplication::platformName();
    return p != "android" && p != "ios" && p != "qnx";
}

QString PlatformUtils::platformName() const {
    return QGuiApplication::platformName();
}

QStringList PlatformUtils::supportedExtensions() const {
    return Settings::supportedExtensions();
}

QString PlatformUtils::fileDialogFilter() const {
    const QStringList exts = Settings::supportedExtensions();
    QStringList globs;
    for (const QString &e : exts)
        globs.append(QString("*.%1").arg(e));
    return QString("Images (%1);;All files (*.*)").arg(globs.join(" "));
}

void PlatformUtils::copyImageToClipboard(const QString &path) {
    const QUrl url(path);
    const QString local = url.isLocalFile() ? url.toLocalFile() : path;
    const QImage img(local);
    if (img.isNull())
        return;
    QMimeData *mime = new QMimeData;
    mime->setImageData(img);
    QGuiApplication::clipboard()->setMimeData(mime, QClipboard::Clipboard);
}

void PlatformUtils::openContainingFolder(const QString &path) {
    const QUrl url(path);
    const QString local = url.isLocalFile() ? url.toLocalFile() : path;
    const QFileInfo info(local);
    const QString dir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();

#if defined(Q_OS_WIN)
    // Use Explorer's "select" so the file is highlighted.
    QProcess::startDetached("explorer.exe",
        {"/select,", QDir::toNativeSeparators(local)});
#elif defined(Q_OS_MACOS)
    QProcess::execute("/usr/bin/osascript",
        {"-e", QString("tell application \"Finder\" to reveal POSIX file \"%1\"")
             .arg(local)});
    QProcess::execute("/usr/bin/osascript",
        {"-e", "tell application \"Finder\" to activate"});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
#endif
}

bool PlatformUtils::setAsWallpaper(const QString &path) {
    if (!isDesktop())
        return false;
    const QUrl url(path);
    const QString local = url.isLocalFile() ? url.toLocalFile() : path;
    if (!QFileInfo::exists(local))
        return false;

#if defined(Q_OS_WIN)
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                              (PVOID)local.toStdWString().c_str(),
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        return true;
    }
    return false;
#elif defined(Q_OS_MACOS)
    const QString script = QString(
        "tell application \"System Events\" to tell every desktop to set picture to POSIX file \"%1\"")
        .arg(local);
    return QProcess::execute("/usr/bin/osascript", {"-e", script}) == 0;
#elif defined(Q_OS_LINUX)
    // Best effort for GNOME / Unity / KDE.
    const QStringList cmds = {
        QString("gsettings set org.gnome.desktop.background picture-uri file://%1").arg(local),
        QString("gsettings set org.gnome.desktop.background picture-uri-dark file://%1").arg(local),
        QString("kwriteconfig5 --group Wallpaper --key Image file://%1").arg(local),
    };
    for (const QString &c : cmds)
        QProcess::execute("/bin/sh", {"-c", c});
    return true;
#else
    Q_UNUSED(local);
    return false;
#endif
}
