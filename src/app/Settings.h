#pragma once

#include <QByteArray>
#include <QStringList>
#include <QVariant>

namespace fv {

class Settings {
public:
    static Settings &instance();

    // theme: "system" | "light" | "dark"
    QString theme() const;
    void setTheme(const QString &theme);

    QString language() const;                 // "auto" | "zh_CN" | "en"
    void setLanguage(const QString &lang);

    bool customTitlebar() const;
    void setCustomTitlebar(bool on);

    bool galleryRecursive() const;
    void setGalleryRecursive(bool on);

    bool viewerSmooth() const;
    void setViewerSmooth(bool on);

    bool showThumbnailBar() const;
    void setShowThumbnailBar(bool on);

    bool showInfoPanel() const;
    void setShowInfoPanel(bool on);

    int slideshowIntervalMs() const;
    void setSlideshowIntervalMs(int ms);

    bool slideshowRandom() const;
    void setSlideshowRandom(bool on);

    QString defaultFitMode() const;           // "fit" | "width" | "actual"
    void setDefaultFitMode(const QString &mode);

    QStringList recentFolders() const;
    void addRecentFolder(const QString &dir);
    void removeRecentFolder(const QString &dir);

    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray &g);

private:
    Settings();
    QVariant value(const QString &key, const QVariant &def) const;
    void setValue(const QString &key, const QVariant &v);
};

} // namespace fv
