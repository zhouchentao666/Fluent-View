#include "Settings.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace fv {

namespace {
constexpr char kGroup[] = "fluentview";
}

Settings &Settings::instance()
{
    static Settings s;
    return s;
}

Settings::Settings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

QVariant Settings::value(const QString &key, const QVariant &def) const
{
    QSettings s;
    s.beginGroup(kGroup);
    return s.value(key, def);
}

void Settings::setValue(const QString &key, const QVariant &v)
{
    QSettings s;
    s.beginGroup(kGroup);
    s.setValue(key, v);
}

QString Settings::theme() const { return value("theme", "system").toString(); }
void Settings::setTheme(const QString &theme) { setValue("theme", theme); }

QString Settings::language() const { return value("language", "auto").toString(); }
void Settings::setLanguage(const QString &lang) { setValue("language", lang); }

bool Settings::customTitlebar() const { return value("customTitlebar", true).toBool(); }
void Settings::setCustomTitlebar(bool on) { setValue("customTitlebar", on); }

bool Settings::galleryRecursive() const { return value("galleryRecursive", false).toBool(); }
void Settings::setGalleryRecursive(bool on) { setValue("galleryRecursive", on); }

bool Settings::viewerSmooth() const { return value("viewerSmooth", true).toBool(); }
void Settings::setViewerSmooth(bool on) { setValue("viewerSmooth", on); }

bool Settings::showThumbnailBar() const { return value("showThumbnailBar", true).toBool(); }
void Settings::setShowThumbnailBar(bool on) { setValue("showThumbnailBar", on); }

bool Settings::showInfoPanel() const { return value("showInfoPanel", false).toBool(); }
void Settings::setShowInfoPanel(bool on) { setValue("showInfoPanel", on); }

int Settings::slideshowIntervalMs() const
{
    return qBound(500, value("slideshowIntervalMs", 3000).toInt(), 60000);
}
void Settings::setSlideshowIntervalMs(int ms) { setValue("slideshowIntervalMs", ms); }

bool Settings::slideshowRandom() const { return value("slideshowRandom", false).toBool(); }
void Settings::setSlideshowRandom(bool on) { setValue("slideshowRandom", on); }

QString Settings::defaultFitMode() const { return value("defaultFitMode", "fit").toString(); }
void Settings::setDefaultFitMode(const QString &mode) { setValue("defaultFitMode", mode); }

QStringList Settings::recentFolders() const
{
    return value("recentFolders", QStringList()).toStringList();
}

void Settings::addRecentFolder(const QString &dir)
{
    const QString abs = QDir(dir).absolutePath();
    QStringList list = recentFolders();
    list.removeAll(abs);
    list.prepend(abs);
    while (list.size() > 12)
        list.removeLast();
    setValue("recentFolders", list);
}

void Settings::removeRecentFolder(const QString &dir)
{
    QStringList list = recentFolders();
    list.removeAll(QDir(dir).absolutePath());
    setValue("recentFolders", list);
}

QByteArray Settings::mainWindowGeometry() const
{
    return value("windowGeometry").toByteArray();
}
void Settings::setMainWindowGeometry(const QByteArray &g) { setValue("windowGeometry", g); }

} // namespace fv
