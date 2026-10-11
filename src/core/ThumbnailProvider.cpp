#include "ThumbnailProvider.h"
#include "ImageDecode.h"

#include <QQuickTextureFactory>
#include <QUrl>

ThumbnailImageResponse::ThumbnailImageResponse(const QString &id,
                                               const QSize &requestedSize,
                                               ImageCache *cache)
    : m_id(id), m_requestedSize(requestedSize), m_cache(cache) {
    QThreadPool::globalInstance()->start(new ThumbnailDecodeTask(this));
}

QQuickTextureFactory *ThumbnailImageResponse::textureFactory() const {
    if (!m_done || m_image.isNull())
        return nullptr;
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

QString ThumbnailImageResponse::errorString() const {
    return m_error;
}

void ThumbnailImageResponse::run() {
    const QString path = QUrl::fromPercentEncoding(m_id.toUtf8());
    const QSize req = m_requestedSize.isEmpty() ? QSize(256, 256) : m_requestedSize;
    // Thumbnails are always upright (no rotation/flip applied).
    m_image = ImageDecode::decode(path, req, 0, false, false, m_cache, &m_error);
    m_done = true;
    emit finished();
}

ThumbnailImageProvider::ThumbnailImageProvider(ImageCache *cache) : m_cache(cache) {}

QQuickImageResponse *ThumbnailImageProvider::requestImageResponse(
    const QString &id, const QSize &requestedSize) {
    return new ThumbnailImageResponse(id, requestedSize, m_cache);
}
