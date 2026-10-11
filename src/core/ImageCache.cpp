#include "ImageCache.h"

ImageCache::ImageCache(QObject *parent) : QObject(parent) {}

void ImageCache::setMaxBytes(qint64 bytes) {
    QMutexLocker lock(&m_mutex);
    m_maxBytes = bytes;
    evict();
}

bool ImageCache::get(const QString &key, QImage &out) {
    QMutexLocker lock(&m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end())
        return false;
    it->lastUsed = ++m_clock;
    out = it->image;
    return true;
}

void ImageCache::insert(const QString &key, const QImage &img) {
    QMutexLocker lock(&m_mutex);
    CacheEntry entry;
    entry.image = img;
    entry.bytes = estimateBytes(img);
    entry.lastUsed = ++m_clock;
    m_cache.insert(key, entry);
    m_used += entry.bytes;
    evict();
}

void ImageCache::clear() {
    QMutexLocker lock(&m_mutex);
    m_cache.clear();
    m_used = 0;
}

qint64 ImageCache::estimateBytes(const QImage &img) const {
    // Account for the bits plus a modest overhead per entry.
    return static_cast<qint64>(img.sizeInBytes()) + 256;
}

void ImageCache::evict() {
    while (m_used > m_maxBytes && !m_cache.isEmpty()) {
        // Find the least-recently-used entry.
        auto oldest = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->lastUsed < oldest->lastUsed)
                oldest = it;
        }
        m_used -= oldest->bytes;
        m_cache.erase(oldest);
    }
}
