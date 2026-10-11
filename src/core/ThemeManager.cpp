#include "ThemeManager.h"
#include "Settings.h"

#include <QGuiApplication>
#include <QStyleHints>

ThemeManager::ThemeManager(Settings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

void ThemeManager::apply() {
    if (!m_settings)
        return;
    const QString t = m_settings->theme();
    QStyleHints *hints = QGuiApplication::styleHints();
    if (t == "light")
        hints->setColorScheme(Qt::ColorScheme::Light);
    else if (t == "dark")
        hints->setColorScheme(Qt::ColorScheme::Dark);
    else
        hints->setColorScheme(Qt::ColorScheme::Unknown); // follow system
    emit themeApplied();
}

QString ThemeManager::effectiveTheme() const {
    switch (QGuiApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Light: return "light";
    case Qt::ColorScheme::Dark:  return "dark";
    default:                     return "system";
    }
}

bool ThemeManager::isDark() const {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}
