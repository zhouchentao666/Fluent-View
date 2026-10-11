#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QRect>

/**
 * Persistent application settings, exposed to QML as a context property.
 */
class Settings : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(int zoomMode READ zoomMode WRITE setZoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(bool rememberWindow READ rememberWindow WRITE setRememberWindow NOTIFY rememberWindowChanged)
    Q_PROPERTY(int maxCacheMB READ maxCacheMB WRITE setMaxCacheMB NOTIFY maxCacheMBChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList defaultFormats READ defaultFormats WRITE setDefaultFormats NOTIFY defaultFormatsChanged)
    Q_PROPERTY(QRect windowGeometry READ windowGeometry WRITE setWindowGeometry NOTIFY windowGeometryChanged)
    Q_PROPERTY(bool thumbnailSidebar READ thumbnailSidebar WRITE setThumbnailSidebar NOTIFY thumbnailSidebarChanged)

public:
    enum ZoomMode { Fit = 0, ActualSize = 1, Stretch = 2 };
    Q_ENUM(ZoomMode)

    explicit Settings(QObject *parent = nullptr);

    QString theme() const;
    void setTheme(const QString &v);

    int zoomMode() const;
    void setZoomMode(int v);

    bool rememberWindow() const;
    void setRememberWindow(bool v);

    int maxCacheMB() const;
    void setMaxCacheMB(int v);

    QString language() const;
    void setLanguage(const QString &v);

    QStringList defaultFormats() const;
    void setDefaultFormats(const QStringList &v);

    QRect windowGeometry() const;
    void setWindowGeometry(const QRect &v);

    bool thumbnailSidebar() const;
    void setThumbnailSidebar(bool v);

    /** Extensions that the viewer can open (lower-case, no dot). */
    static QStringList supportedExtensions();

signals:
    void themeChanged();
    void zoomModeChanged();
    void rememberWindowChanged();
    void maxCacheMBChanged();
    void languageChanged();
    void defaultFormatsChanged();
    void windowGeometryChanged();
    void thumbnailSidebarChanged();

private:
    QSettings m_settings;
};
