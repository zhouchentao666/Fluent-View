#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QThreadPool>
#include <QRunnable>
#include <QImage>

#include "ImageCache.h"

/**
 * Asynchronous, off-GUI-thread provider for the *main* image.
 *
 * QML requests e.g.:
 *   image://viewer/<percentEncodedPath>?r=90&fh=1&fv=0
 *
 * The decode runs in a thread-pool worker and the result is uploaded to a GPU
 * texture by the scene graph, so the UI thread is never blocked and huge images
 * are only ever decoded at the size requested via Image.sourceSize.
 */
class ViewerImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    ViewerImageResponse(const QString &id, const QSize &requestedSize, ImageCache *cache);
    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;

    void run(); // executed in worker thread

    QImage m_image;
    QString m_error;
    bool m_done = false;

private:
    QString m_id;
    QSize m_requestedSize;
    ImageCache *m_cache;
};

class ViewerImageProvider : public QQuickAsyncImageProvider {
public:
    explicit ViewerImageProvider(ImageCache *cache = nullptr);
    QQuickImageResponse *requestImageResponse(const QString &id,
                                             const QSize &requestedSize) override;
    void setCache(ImageCache *cache) { m_cache = cache; }

private:
    ImageCache *m_cache = nullptr;
};

// Worker that drives ViewerImageResponse::run() on a thread-pool thread.
class ViewerDecodeTask : public QRunnable {
public:
    explicit ViewerDecodeTask(ViewerImageResponse *r) : m_resp(r) {}
    void run() override { m_resp->run(); }
private:
    ViewerImageResponse *m_resp;
};
