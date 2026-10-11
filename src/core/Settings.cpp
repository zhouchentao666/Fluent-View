#include "Settings.h"

Settings::Settings(QObject *parent) : QObject(parent) {}

QString Settings::theme() const { return m_settings.value("theme", "system").toString(); }
void Settings::setTheme(const QString &v) {
    if (v == theme()) return;
    m_settings.setValue("theme", v);
    emit themeChanged();
}

int Settings::zoomMode() const { return m_settings.value("zoomMode", Fit).toInt(); }
void Settings::setZoomMode(int v) {
    if (v == zoomMode()) return;
    m_settings.setValue("zoomMode", v);
    emit zoomModeChanged();
}

bool Settings::rememberWindow() const { return m_settings.value("rememberWindow", true).toBool(); }
void Settings::setRememberWindow(bool v) {
    if (v == rememberWindow()) return;
    m_settings.setValue("rememberWindow", v);
    emit rememberWindowChanged();
}

int Settings::maxCacheMB() const { return m_settings.value("maxCacheMB", 512).toInt(); }
void Settings::setMaxCacheMB(int v) {
    if (v == maxCacheMB()) return;
    m_settings.setValue("maxCacheMB", v);
    emit maxCacheMBChanged();
}

QString Settings::language() const { return m_settings.value("language", "system").toString(); }
void Settings::setLanguage(const QString &v) {
    if (v == language()) return;
    m_settings.setValue("language", v);
    emit languageChanged();
}

QStringList Settings::defaultFormats() const {
    return m_settings.value("defaultFormats", supportedExtensions()).toStringList();
}
void Settings::setDefaultFormats(const QStringList &v) {
    if (v == defaultFormats()) return;
    m_settings.setValue("defaultFormats", v);
    emit defaultFormatsChanged();
}

QRect Settings::windowGeometry() const { return m_settings.value("windowGeometry", QRect()).toRect(); }
void Settings::setWindowGeometry(const QRect &v) {
    m_settings.setValue("windowGeometry", v);
    emit windowGeometryChanged();
}

bool Settings::thumbnailSidebar() const { return m_settings.value("thumbnailSidebar", true).toBool(); }
void Settings::setThumbnailSidebar(bool v) {
    if (v == thumbnailSidebar()) return;
    m_settings.setValue("thumbnailSidebar", v);
    emit thumbnailSidebarChanged();
}

QStringList Settings::supportedExtensions() {
    return {"jpg", "jpeg", "png", "webp", "avif", "bmp", "gif", "heic", "heif", "tiff", "tif", "svg"};
}
