#include "ThemeManager.h"

#include "Settings.h"

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include <QWidget>

#ifdef Q_OS_WIN
#  include <dwmapi.h>
#  include <windows.h>
#  pragma comment(lib, "dwmapi.lib")
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#  ifndef DWMWA_SYSTEMBACKDROP_TYPE
#    define DWMWA_SYSTEMBACKDROP_TYPE 38
#  endif
#  ifndef DWMSBT_MAINWINDOW
#    define DWMSBT_MAINWINDOW 2   // Mica
#  endif
#endif

namespace fv {

ThemeManager &ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    m_preference = Settings::instance().theme();
}

void ThemeManager::detectSystemDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    const bool hintsDark = scheme == Qt::ColorScheme::Dark;
#else
    const QPalette palette = QGuiApplication::palette();
    const bool hintsDark = palette.color(QPalette::Window).lightness() < 128;
#endif
    m_dark = (m_preference == QLatin1String("dark"))
             || (m_preference == QLatin1String("system") && hintsDark);
}

void ThemeManager::setPreference(const QString &theme)
{
    if (m_preference == theme)
        return;
    m_preference = theme;
    Settings::instance().setTheme(theme);
    apply();
}

QIcon ThemeManager::icon(const QString &name) const
{
    const QString root = m_dark ? QStringLiteral(":/icons/dark") : QStringLiteral(":/icons/light");
    QIcon ic(root + QLatin1Char('/') + name + QStringLiteral(".svg"));
    if (ic.isNull())
        ic = QIcon(QStringLiteral(":/icons/fluentview.svg"));
    return ic;
}

void ThemeManager::loadQss(bool dark)
{
    const QString path = dark ? QStringLiteral(":/styles/dark.qss") : QStringLiteral(":/styles/light.qss");
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
}

void ThemeManager::apply()
{
    detectSystemDark();
    loadQss(m_dark);

    QPalette pal;
    if (m_dark) {
        pal.setColor(QPalette::Window,          QColor(0x20, 0x20, 0x20));
        pal.setColor(QPalette::WindowText,      QColor(0xF3, 0xF3, 0xF3));
        pal.setColor(QPalette::Base,            QColor(0x27, 0x27, 0x27));
        pal.setColor(QPalette::AlternateBase,   QColor(0x2D, 0x2D, 0x2D));
        pal.setColor(QPalette::Text,            QColor(0xF3, 0xF3, 0xF3));
        pal.setColor(QPalette::Button,          QColor(0x2B, 0x2B, 0x2B));
        pal.setColor(QPalette::ButtonText,      QColor(0xF3, 0xF3, 0xF3));
        pal.setColor(QPalette::Highlight,       QColor(0x4C, 0xC2, 0xFF));
        pal.setColor(QPalette::HighlightedText, QColor(0x10, 0x10, 0x10));
        pal.setColor(QPalette::ToolTipBase,     QColor(0x2B, 0x2B, 0x2B));
        pal.setColor(QPalette::ToolTipText,     QColor(0xF3, 0xF3, 0xF3));
        pal.setColor(QPalette::PlaceholderText, QColor(0x9D, 0x9D, 0x9D));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x77, 0x77, 0x77));
    } else {
        pal.setColor(QPalette::Window,          QColor(0xF3, 0xF3, 0xF3));
        pal.setColor(QPalette::WindowText,      QColor(0x1B, 0x1B, 0x1B));
        pal.setColor(QPalette::Base,            QColor(0xFF, 0xFF, 0xFF));
        pal.setColor(QPalette::AlternateBase,   QColor(0xF7, 0xF7, 0xF7));
        pal.setColor(QPalette::Text,            QColor(0x1B, 0x1B, 0x1B));
        pal.setColor(QPalette::Button,          QColor(0xFB, 0xFB, 0xFB));
        pal.setColor(QPalette::ButtonText,      QColor(0x1B, 0x1B, 0x1B));
        pal.setColor(QPalette::Highlight,       QColor(0x00, 0x5F, 0xB7));
        pal.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
        pal.setColor(QPalette::ToolTipBase,     QColor(0xFF, 0xFF, 0xFF));
        pal.setColor(QPalette::ToolTipText,     QColor(0x1B, 0x1B, 0x1B));
        pal.setColor(QPalette::PlaceholderText, QColor(0x70, 0x70, 0x70));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x9D, 0x9D, 0x9D));
    }
    qApp->setPalette(pal);

    emit themeChanged(m_dark);
}

void ThemeManager::polishWindow(QWidget *window)
{
#ifdef Q_OS_WIN
    if (!window || !window->windowHandle())
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL dark = m_dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // Mica on Windows 11; silently ignored on older systems.
    const INT backdrop = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    // Windows 11 rounded corners preference (2 = round).
    const INT round = 2;
    DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &round, sizeof(round));
#else
    Q_UNUSED(window);
#endif
}

} // namespace fv
