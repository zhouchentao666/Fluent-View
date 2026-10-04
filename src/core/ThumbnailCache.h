#pragma once

#include <QCache>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>
#include <QString>

namespace fv {

// Two-level thumbnail cache: in-memory LRU + on-disk PNG store under
// CacheLocation/thumbnails. Requests run on worker threads and are delivered
// through the ready() signal on the GUI thread.
class ThumbnailCache : public QObject {
    Q_OBJECT
public:
    static ThumbnailCache &instance();

    void request(const QString &path, int edge = 320);
    QImage cached(const QString &path, int edge) const;
    void clearDiskCache();
    void evictMemory() { m_memory.clear(); }

signals:
    void ready(const QString &path, const QImage &thumb);

private:
    explicit ThumbnailCache(QObject *parent = nullptr);

    QString diskPath(const QString &key) const;
    static QString keyFor(const QString &path, int edge);

    struct Entry { QImage image; };

    mutable QReadWriteLock m_lock;
    QCache<QString, Entry> m_memory;
    QSet<QString>          m_inFlight;
    QString                m_diskDir;
};

} // namespace fv
