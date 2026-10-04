#pragma once

#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace fv {

// Bottom floating pill toolbar: zoom out/label/in, fit modes, rotate/flip,
// slideshow, and info toggles. Buttons are wired by MainWindow.
class FloatingToolbar : public QWidget {
    Q_OBJECT
public:
    explicit FloatingToolbar(QWidget *parent = nullptr);

    QPushButton *add(const QString &iconName, const QString &tooltip);
    void setZoomText(const QString &text);
    void setInfoText(const QString &text);

    QPushButton *btnZoomOut() const { return m_zoomOut; }
    QPushButton *btnZoomIn() const { return m_zoomIn; }
    QPushButton *btnFit() const { return m_fit; }
    QPushButton *btnActual() const { return m_actual; }
    QPushButton *btnRotateLeft() const { return m_rotateLeft; }
    QPushButton *btnRotateRight() const { return m_rotateRight; }
    QPushButton *btnFlipH() const { return m_flipH; }
    QPushButton *btnFlipV() const { return m_flipV; }
    QPushButton *btnSlideshow() const { return m_slideshow; }
    QPushButton *btnInfo() const { return m_info; }
    QPushButton *btnThumbs() const { return m_thumbs; }
    QPushButton *btnMenu() const { return m_menu; }

private:
    QPushButton *m_zoomOut = nullptr;
    QPushButton *m_zoomIn = nullptr;
    QPushButton *m_fit = nullptr;
    QPushButton *m_actual = nullptr;
    QPushButton *m_rotateLeft = nullptr;
    QPushButton *m_rotateRight = nullptr;
    QPushButton *m_flipH = nullptr;
    QPushButton *m_flipV = nullptr;
    QPushButton *m_slideshow = nullptr;
    QPushButton *m_info = nullptr;
    QPushButton *m_thumbs = nullptr;
    QPushButton *m_menu = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
    QHBoxLayout *m_layout = nullptr;
};

} // namespace fv
