#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QThreadPool>
#include <QRunnable>
#include <QImage>

#include "ImageCache.h"

/**
 * Asynchronous thumbnail provider. Thumbnails are generated off the GUI thread
 * and cached. QML requests:
 *   image://thumb/<percentEncodedPath>
 */
class ThumbnailImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    ThumbnailImageResponse(const QString &id, const QSize &requestedSize, ImageCache *cache);
    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;

    void run();

    QImage m_image;
    QString m_error;
    bool m_done = false;

private:
    QString m_id;
    QSize m_requestedSize;
    ImageCache *m_cache;
};

class ThumbnailImageProvider : public QQuickAsyncImageProvider {
public:
    explicit ThumbnailImageProvider(ImageCache *cache = nullptr);
    QQuickImageResponse *requestImageResponse(const QString &id,
                                             const QSize &requestedSize) override;
    void setCache(ImageCache *cache) { m_cache = cache; }

private:
    ImageCache *m_cache = nullptr;
};

class ThumbnailDecodeTask : public QRunnable {
public:
    explicit ThumbnailDecodeTask(ThumbnailImageResponse *r) : m_resp(r) {}
    void run() override { m_resp->run(); }
private:
    ThumbnailImageResponse *m_resp;
};
