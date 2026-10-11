#pragma once

#include <QObject>
#include <QString>

/**
 * Desktop / platform integration helpers exposed to QML:
 * clipboard, "open containing folder", "set as wallpaper", file associations.
 */
class PlatformUtils : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isDesktop READ isDesktop CONSTANT)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)
    Q_PROPERTY(QStringList supportedExtensions READ supportedExtensions CONSTANT)
    Q_PROPERTY(QString fileDialogFilter READ fileDialogFilter CONSTANT)

public:
    explicit PlatformUtils(QObject *parent = nullptr);

    Q_INVOKABLE bool isDesktop() const;
    Q_INVOKABLE QString platformName() const;
    Q_INVOKABLE QStringList supportedExtensions() const;
    Q_INVOKABLE QString fileDialogFilter() const;

    Q_INVOKABLE void copyImageToClipboard(const QString &path);
    Q_INVOKABLE void openContainingFolder(const QString &path);
    Q_INVOKABLE bool setAsWallpaper(const QString &path);
};
