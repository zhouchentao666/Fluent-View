#include "ImageController.h"

#include <QImageReader>
#include <QFileInfo>
#include <QUrl>

ImageController::ImageController(QObject *parent) : QObject(parent) {}

void ImageController::setPath(const QString &p) {
    const QString local = QUrl(p).isLocalFile() ? QUrl(p).toLocalFile() : p;
    if (local == m_path)
        return;
    m_path = local;
    readMetadata();
    emit pathChanged();
    emit displayChanged();
}

QString ImageController::source() const {
    if (m_path.isEmpty())
        return {};
    return QString("image://viewer/%1?r=%2&fh=%3&fv=%4")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_path)))
        .arg(m_rotation)
        .arg(m_flipH ? 1 : 0)
        .arg(m_flipV ? 1 : 0);
}

void ImageController::readMetadata() {
    m_imgW = m_imgH = 0;
    m_fileSize = 0;
    m_format.clear();
    m_fileName.clear();

    if (m_path.isEmpty())
        return;

    const QFileInfo info(m_path);
    m_fileName = info.fileName();
    m_fileSize = info.size();

    QImageReader reader(m_path);
    reader.setAutoTransform(true);
    const QSize s = reader.size();
    m_imgW = s.width();
    m_imgH = s.height();
    m_format = reader.format().toUpper();

    emit infoChanged();
}

void ImageController::rotateLeft() {
    m_rotation = (m_rotation + 270) % 360;
    emit transformChanged();
    emit displayChanged();
}

void ImageController::rotateRight() {
    m_rotation = (m_rotation + 90) % 360;
    emit transformChanged();
    emit displayChanged();
}

void ImageController::flipHorizontal() {
    m_flipH = !m_flipH;
    emit transformChanged();
    emit displayChanged();
}

void ImageController::flipVertical() {
    m_flipV = !m_flipV;
    emit transformChanged();
    emit displayChanged();
}

void ImageController::resetTransform() {
    if (m_rotation == 0 && !m_flipH && !m_flipV)
        return;
    m_rotation = 0;
    m_flipH = m_flipV = false;
    emit transformChanged();
    emit displayChanged();
}

void ImageController::toggleFullscreen() {
    emit toggleFullscreenRequested();
}
