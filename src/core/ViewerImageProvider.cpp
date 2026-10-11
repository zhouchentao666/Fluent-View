#include "ViewerImageProvider.h"
#include "ImageDecode.h"

#include <QQuickTextureFactory>
#include <QUrl>

ViewerImageResponse::ViewerImageResponse(const QString &id,
                                         const QSize &requestedSize,
                                         ImageCache *cache)
    : m_id(id), m_requestedSize(requestedSize), m_cache(cache) {
    QThreadPool::globalInstance()->start(new ViewerDecodeTask(this));
}

QQuickTextureFactory *ViewerImageResponse::textureFactory() const {
    if (!m_done || m_image.isNull())
        return nullptr;
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

QString ViewerImageResponse::errorString() const {
    return m_error;
}

void ViewerImageResponse::run() {
    // Parse "path?r=90&fh=1&fv=0"
    const int qpos = m_id.indexOf('?');
    QString path = (qpos >= 0) ? m_id.left(qpos) : m_id;
    path = QUrl::fromPercentEncoding(path.toUtf8());

    int rotation = 0;
    bool flipH = false, flipV = false;
    if (qpos >= 0) {
        const QString query = m_id.mid(qpos + 1);
        const auto parts = query.split('&', Qt::SkipEmptyParts);
        for (const auto &p : parts) {
            if (p.startsWith("r=")) rotation = p.mid(2).toInt();
            else if (p.startsWith("fh=")) flipH = (p.mid(3).toInt() != 0);
            else if (p.startsWith("fv=")) flipV = (p.mid(3).toInt() != 0);
        }
    }

    m_image = ImageDecode::decode(path, m_requestedSize, rotation, flipH, flipV,
                                  m_cache, &m_error);
    m_done = true;
    emit finished();
}

ViewerImageProvider::ViewerImageProvider(ImageCache *cache) : m_cache(cache) {}

QQuickImageResponse *ViewerImageProvider::requestImageResponse(
    const QString &id, const QSize &requestedSize) {
    return new ViewerImageResponse(id, requestedSize, m_cache);
}
