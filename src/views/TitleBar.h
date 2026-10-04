#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QPushButton>
#include <QWidget>

namespace fv {

// Custom title bar for frameless windows: draggable, double-click to
// maximize, min/max/close buttons. On macOS we leave the native traffic
// lights alone (window flags) and only render the title text.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setDark(bool dark);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void setupWindowsControls();

    QLabel *m_title = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QWidget *m_controls = nullptr;
    QPoint   m_dragStart;
    bool     m_dragging = false;
};

} // namespace fv
