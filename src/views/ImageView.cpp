#include "views/ImageView.h"

#include <QFileInfo>
#include <QGestureEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QTransform>
#include <QWheelEvent>

#include <cmath>

namespace fv {

namespace {
constexpr qreal kMinZoom = 0.01;
constexpr qreal kMaxZoom = 32.0;
}

ImageView::ImageView(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(false);
    setFocusPolicy(Qt::ClickFocus);

    grabGesture(Qt::PinchGesture);

    m_fade.setDuration(180);
    m_slide.setDuration(200);
    m_zoomAnim.setDuration(180);
    m_zoomAnim.setEasingCurve(QEasingCurve::OutCubic);

    m_movieRetry.setSingleShot(true);
    m_movieRetry.setInterval(200);
    connect(&m_movieRetry, &QTimer::timeout, this, &ImageView::tryStartMovie);

    connect(&m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_fadeValue = v.toReal();
        update();
    });
    connect(&m_slide, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_slideValue = v.toPointF();
        update();
    });
    connect(&m_zoomAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_zoom = clampZoom(v.toReal());
        update();
        emit zoomChanged(m_zoom);
    });
}

// --------------------------------------------------------------------------
// Image state
// --------------------------------------------------------------------------
void ImageView::setSmoothScaling(bool on)
{
    m_smooth = on;
    update();
}

void ImageView::setImage(const LoadResult &result)
{
    // keep the outgoing frame for the cross-fade
    if (!m_image.isNull())
        m_prevImage = m_image;
    m_sourceImage = result.image;
    m_image = m_sourceImage;
    m_currentPath = result.path;
    m_showingError = false;
    m_errorInfo.clear();

    if (m_movie) {
        m_movie->stop();
        m_movie.reset();
    }

    startSwitchAnimation();
    applyFit(m_fit, false);
    emit sizeChanged(m_image.size());
    tryStartMovie();
}

void ImageView::showError(const QString &path, const QString &message, const QStringList &infoRows)
{
    m_showingError = true;
    m_errorText = message;
    m_errorInfo = infoRows;
    m_currentPath = path;
    if (m_movie) {
        m_movie->stop();
        m_movie.reset();
    }
    m_image = QImage();
    m_prevImage = QImage();
    m_fade.setCurrentTime(0);
    m_fade.setCurrentTime(int(m_fade.totalDuration()));
    update();
}

void ImageView::tryStartMovie()
{
    // Animated formats: replace the static first frame with a QMovie. Needs
    // the widget to be visible; retry briefly otherwise.
    if (m_movie || m_showingError || m_currentPath.isEmpty() || m_image.isNull())
        return;
    if (!isVisible()) {
        m_movieRetry.start();
        return;
    }

    const QString suffix = QFileInfo(m_currentPath).suffix().toLower();
    const bool candidate = suffix == QLatin1String("gif") ||
                           suffix == QLatin1String("mng") ||
                           suffix == QLatin1String("webp") ||
                           suffix == QLatin1String("apng") ||
                           suffix == QLatin1String("png") ||
                           suffix == QLatin1String("svg") ||
                           suffix == QLatin1String("svgz");
    if (!candidate)
        return;

    auto movie = std::make_unique<QMovie>(m_currentPath);
    if (!movie->isValid())
        return;
    if (movie->frameCount() == 1 &&
        suffix != QLatin1String("svg") && suffix != QLatin1String("svgz")) {
        return;   // single-frame file: keep the static QImage
    }

    connect(movie.get(), &QMovie::frameChanged, this, [this](int) {
        if (m_movie) {
            m_image = m_movie->currentImage();
            update();
        }
    });
    movie->start();
    m_movie = std::move(movie);
    update();
}

void ImageView::startSwitchAnimation()
{
    if (!m_prevImage.isNull() && !m_image.isNull()) {
        m_fade.stop();
        m_fade.setStartValue(0.0);
        m_fade.setEndValue(1.0);
        m_fade.start();

        m_slide.stop();
        m_slide.setStartValue(QPointF(24, 0));
        m_slide.setEndValue(QPointF(0, 0));
        m_slide.start();
    } else {
        m_fade.setCurrentTime(0);
        m_fade.setCurrentTime(int(m_fade.totalDuration()));
        m_slide.setCurrentTime(0);
        m_slide.setCurrentTime(int(m_slide.totalDuration()));
    }
}

// --------------------------------------------------------------------------
// Transformations
// --------------------------------------------------------------------------
qreal ImageView::clampZoom(qreal z) const
{
    return qBound(kMinZoom, z, kMaxZoom);
}

QRectF ImageView::drawRect() const
{
    if (m_image.isNull())
        return QRectF();
    return QRectF(m_offset, QSizeF(m_image.width() * m_zoom, m_image.height() * m_zoom));
}

void ImageView::applyFit(FitMode mode, bool animate)
{
    if (m_image.isNull())
        return;
    m_fit = mode;

    const QSize view = size();
    qreal newZoom = m_zoom;
    switch (mode) {
    case FitMode::Fit:
        newZoom = qMin(qreal(view.width()) / m_image.width(),
                       qreal(view.height()) / m_image.height());
        break;
    case FitMode::FitWidth:
        newZoom = qreal(view.width()) / m_image.width();
        break;
    case FitMode::ActualSize:
        newZoom = 1.0;
        break;
    }
    newZoom = clampZoom(newZoom);

    if (animate) {
        m_zoomAnim.stop();
        m_zoomAnim.setStartValue(m_zoom);
        m_zoomAnim.setEndValue(newZoom);
        disconnect(m_zoomAnimFinishedConn);
        m_zoomAnimFinishedConn = connect(&m_zoomAnim, &QVariantAnimation::finished, this,
                                         [this]() { centerImage(); update(); });
        m_zoomAnim.start();
    } else {
        m_zoom = newZoom;
        centerImage();
        update();
        emit zoomChanged(m_zoom);
    }
}

void ImageView::fitWindow()  { applyFit(FitMode::Fit, true); }
void ImageView::fitWidth()   { applyFit(FitMode::FitWidth, true); }
void ImageView::actualSize() { applyFit(FitMode::ActualSize, true); }

void ImageView::centerImage()
{
    if (m_image.isNull())
        return;
    m_offset = QPointF((width() - m_image.width() * m_zoom) / 2.0,
                       (height() - m_image.height() * m_zoom) / 2.0);
}

void ImageView::setZoomValue(qreal factor)
{
    m_zoom = clampZoom(factor);
    update();
    emit zoomChanged(m_zoom);
}

void ImageView::setZoom(qreal factor)
{
    const QPointF center = QPointF(width(), height()) / 2.0;
    const QPointF before = (center - m_offset) / m_zoom;
    m_zoom = clampZoom(factor);
    m_offset = center - before * m_zoom;
    m_fit = FitMode::ActualSize;
    update();
    emit zoomChanged(m_zoom);
}

void ImageView::zoomIn()  { setZoom(m_zoom * 1.25); }
void ImageView::zoomOut() { setZoom(m_zoom / 1.25); }

void ImageView::rotateLeft()
{
    m_rotation = std::fmod(m_rotation + 270.0, 360.0);
    applyOrientation();
}

void ImageView::rotateRight()
{
    m_rotation = std::fmod(m_rotation + 90.0, 360.0);
    applyOrientation();
}

void ImageView::flipHorizontal()
{
    m_flipH = !m_flipH;
    applyOrientation();
}

void ImageView::flipVertical()
{
    m_flipV = !m_flipV;
    applyOrientation();
}

void ImageView::resetTransform()
{
    m_rotation = 0;
    m_flipH = m_flipV = false;
    applyOrientation();
    applyFit(FitMode::Fit, true);
}

void ImageView::applyOrientation()
{
    QTransform t;
    t.rotate(m_rotation);
    t.scale(m_flipH ? -1.0 : 1.0, m_flipV ? -1.0 : 1.0);
    m_image = m_sourceImage.transformed(t, Qt::SmoothTransformation);
    applyFit(m_fit, false);
    emit sizeChanged(m_image.size());
}

// --------------------------------------------------------------------------
// Events
// --------------------------------------------------------------------------
void ImageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    if (m_showingError) {
        p.setPen(palette().color(QPalette::WindowText));
        p.drawText(rect().adjusted(32, 32, -32, -32), Qt::AlignCenter,
                   tr("Cannot open this file\n\n%1\n\n%2")
                       .arg(m_errorText, m_errorInfo.join(QLatin1Char('\n'))));
        return;
    }
    if (m_image.isNull())
        return;

    // checkerboard behind transparent images
    if (m_image.hasAlphaChannel()) {
        const int cell = 12;
        const QColor a = palette().color(QPalette::Window).lightness() < 128
                ? QColor(0x33, 0x33, 0x33) : QColor(0xEA, 0xEA, 0xEA);
        const QColor b = palette().color(QPalette::Window).lightness() < 128
                ? QColor(0x3D, 0x3D, 0x3D) : QColor(0xDA, 0xDA, 0xDA);
        for (int y = 0; y < height(); y += cell)
            for (int x = 0; x < width(); x += cell)
                p.fillRect(x, y, cell, cell, ((x / cell + y / cell) & 1) ? a : b);
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);
    const QRectF rect = drawRect();

    // outgoing frame during the switch transition
    if (!m_prevImage.isNull() && m_fadeValue < 1.0) {
        const QSizeF ps(m_prevImage.width() * m_zoom, m_prevImage.height() * m_zoom);
        p.setOpacity(1.0 - m_fadeValue);
        p.drawImage(QRectF(m_offset - m_slideValue, ps), m_prevImage);
    }

    p.setOpacity(m_fadeValue);
    p.drawImage(QRectF(m_offset + m_slideValue, rect.size()), m_image);
    p.setOpacity(1.0);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (m_image.isNull())
        return;
    if (event->modifiers() & Qt::ControlModifier) {
        // zoom around cursor
        const QPointF cursor = event->position();
        const QPointF before = (cursor - m_offset) / m_zoom;
        const qreal delta = event->angleDelta().y() / 120.0;
        m_zoom = clampZoom(m_zoom * std::pow(1.25, delta));
        m_offset = cursor - before * m_zoom;
        m_fit = FitMode::ActualSize;
        update();
        emit zoomChanged(m_zoom);
    } else {
        m_offset += QPointF(event->angleDelta().x(), event->angleDelta().y());
        update();
    }
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_image.isNull()) {
        m_panning = true;
        m_panStart = event->pos() - m_offset.toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        m_offset = event->pos() - m_panStart;
        update();
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_panning = false;
    unsetCursor();
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked();
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_fit != FitMode::ActualSize)
        applyFit(m_fit, false);
}

bool ImageView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        auto *ge = static_cast<QGestureEvent *>(event);
        if (QGesture *pinch = ge->gesture(Qt::PinchGesture)) {
            auto *pg = static_cast<QPinchGesture *>(pinch);
            if (pg->changeFlags() & QPinchGesture::ScaleFactorChanged)
                setZoom(m_zoom * pg->scaleFactor());
            return true;
        }
    }
    return QWidget::event(event);
}

} // namespace fv
