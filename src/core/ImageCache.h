#pragma once

#include <QObject>
#include <QImage>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QSize>

/**
 * Thread-safe LRU cache for decoded QImages.
 *
 * Used by the async image providers so that re-decoding (e.g. when the QML
 * Image element is re-evaluated, or when navigating back) is avoided. The total
 * memory footprint is bounded by the configured maximum (see setMaxBytes).
 */
struct CacheEntry {
    QImage image;
    qint64 bytes = 0;
    qint64 lastUsed = 0;
};

class ImageCache : public QObject {
    Q_OBJECT
public:
    explicit ImageCache(QObject *parent = nullptr);

    void setMaxBytes(qint64 bytes);

    bool get(const QString &key, QImage &out);
    void insert(const QString &key, const QImage &img);
    void clear();
    qint64 usedBytes() const { return m_used; }

private:
    void evict();
    qint64 estimateBytes(const QImage &img) const;

    mutable QMutex m_mutex;
    QHash<QString, CacheEntry> m_cache;
    qint64 m_maxBytes = 512 * 1024 * 1024;
    qint64 m_used = 0;
    qint64 m_clock = 0;
};
