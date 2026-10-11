#pragma once

#include <QObject>
#include <QString>

class Settings;

/**
 * Applies the active color scheme to the Qt application style hints.
 * The FluentWinUI3 style reads Qt::ColorScheme, so forcing light/dark (or
 * leaving it "unknown" to follow the OS) gives us theme switching without a
 * restart. Also reports the *effective* scheme for QML-side color decisions.
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(Settings *settings, QObject *parent = nullptr);

    Q_INVOKABLE void apply();
    Q_INVOKABLE QString effectiveTheme() const;
    Q_INVOKABLE bool isDark() const;

signals:
    void themeApplied();

private:
    Settings *m_settings;
};
