#pragma once

#include <QIcon>
#include <QObject>
#include <QString>

namespace fv {

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager &instance();

    void setPreference(const QString &theme);   // "system" | "light" | "dark"
    QString preference() const { return m_preference; }
    bool isDark() const { return m_dark; }

    // Theme-aware monochrome icons: ":/icons/dark/x.svg" or ":/icons/light/x.svg".
    QIcon icon(const QString &name) const;

    // QSS/palette application and platform backdrop (Mica etc.).
    void apply();

    // Call after the window is shown (needs a native winId for DWM calls).
    void polishWindow(class QWidget *window);

signals:
    void themeChanged(bool dark);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    void detectSystemDark();
    void loadQss(bool dark);

    QString m_preference = QStringLiteral("system");
    bool    m_dark = false;
};

} // namespace fv
