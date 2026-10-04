#pragma once

#include "core/ImageTypes.h"

#include <QImage>
#include <QMetaObject>
#include <QMovie>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include <memory>

namespace fv {

// Interactive image canvas: ctrl-wheel/pinch zoom, drag pan, animated fit
// transitions, rotate/flip, fade-slide image switching, and animated formats
// (GIF/WebP/APNG) via QMovie.
class ImageView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoomValue WRITE setZoomValue)
public:
    enum class FitMode { Fit, FitWidth, ActualSize };

    explicit ImageView(QWidget *parent = nullptr);

    qreal zoomValue() const { return m_zoom; }
    bool isShowingError() const { return m_showingError; }

public slots:
    void setSmoothScaling(bool on);
    void setImage(const fv::LoadResult &result);
    void showError(const QString &path, const QString &message, const QStringList &infoRows);

    void zoomIn();
    void zoomOut();
    void setZoom(qreal factor);
    void setZoomValue(qreal factor);   // for the zoom animation property
    void fitWindow();
    void fitWidth();
    void actualSize();
    void rotateLeft();
    void rotateRight();
    void flipHorizontal();
    void flipVertical();
    void resetTransform();

signals:
    void zoomChanged(qreal factor);
    void sizeChanged(const QSize &pixelSize);
    void doubleClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;   // pinch gesture

private:
    void applyFit(FitMode mode, bool animate);
    void startSwitchAnimation();
    void tryStartMovie();
    void applyOrientation();
    qreal clampZoom(qreal z) const;
    void centerImage();
    QRectF drawRect() const;

    QImage  m_sourceImage;       // as loaded
    QImage  m_image;             // oriented (rotation/flip applied)
    QImage  m_prevImage;         // outgoing frame during transition
    QString m_currentPath;
    QString m_errorText;
    QStringList m_errorInfo;
    bool    m_showingError = false;

    FitMode m_fit = FitMode::Fit;
    qreal   m_zoom = 1.0;
    QPointF m_offset;            // image top-left in widget coordinates
    QPoint  m_panStart;
    bool    m_panning = false;
    qreal   m_rotation = 0;      // degrees, multiples of 90
    bool    m_flipH = false;
    bool    m_flipV = false;
    bool    m_smooth = true;

    std::unique_ptr<QMovie> m_movie;
    QTimer                  m_movieRetry;

    QVariantAnimation m_fade;        // 0 -> 1 cross-fade
    QVariantAnimation m_slide;       // slide-in offset
    QVariantAnimation m_zoomAnim;    // animated fit
    QMetaObject::Connection m_zoomAnimFinishedConn;
    qreal   m_fadeValue = 1.0;
    QPointF m_slideValue;
};

} // namespace fv
