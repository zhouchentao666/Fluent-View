#include "ThumbnailCache.h"

#include "ImageLoader.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThreadPool>

namespace fv {

ThumbnailCache &ThumbnailCache::instance()
{
    static ThumbnailCache cache;
    return cache;
}

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
    , m_memory(256)   // up to 256 thumbnails in RAM (default cost = 1 per entry)
{
    m_diskDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + QStringLiteral("/thumbnails");
    QDir().mkpath(m_diskDir);
}

QString ThumbnailCache::keyFor(const QString &path, int edge)
{
    const QFileInfo fi(path);
    const QString identity = QStringLiteral("%1|%2|%3")
            .arg(fi.absoluteFilePath(), QString::number(fi.lastModified().toSecsSinceEpoch()),
                 QString::number(edge));
    return QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QString ThumbnailCache::diskPath(const QString &key) const
{
    return m_diskDir + QLatin1Char('/') + key + QStringLiteral(".png");
}

QImage ThumbnailCache::cached(const QString &path, int edge) const
{
    QReadLocker locker(&m_lock);
    if (Entry *e = m_memory.object(keyFor(path, edge)))
        return e->image;
    return QImage();
}

void ThumbnailCache::request(const QString &path, int edge)
{
    const QString key = keyFor(path, edge);
    {
        QWriteLocker locker(&m_lock);
        if (m_memory.contains(key) || m_inFlight.contains(key))
            return;
        m_inFlight.insert(key);
    }

    const QString diskFile = diskPath(key);
    const QString p = path;
    QThreadPool::globalInstance()->start([this, p, edge, key, diskFile]() {
        QImage img;
        // 1) disk cache
        QFile f(diskFile);
        if (f.open(QIODevice::ReadOnly) && img.load(&f, "PNG")) {
            f.close();
        } else {
            // 2) decode from source
            QString err;
            img = ImageLoader::preview(p, QSize(edge, edge), &err);
            if (!img.isNull()) {
                QSaveFile out(diskFile);
                if (out.open(QIODevice::WriteOnly) && img.save(&out, "PNG"))
                    out.commit();
            }
        }

        QMetaObject::invokeMethod(this, [this, p, edge, key, img]() {
            {
                QWriteLocker locker(&m_lock);
                m_inFlight.remove(key);
                m_memory.insert(key, new Entry{ img }, 1);
            }
            emit ready(p, img);
        }, Qt::QueuedConnection);
    });
}

void ThumbnailCache::clearDiskCache()
{
    QWriteLocker locker(&m_lock);
    m_memory.clear();
    locker.unlock();
    QDir(m_diskDir).removeRecursively();
    QDir().mkpath(m_diskDir);
}

} // namespace fv
