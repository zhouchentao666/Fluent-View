#include "views/GalleryModel.h"

#include "core/ThumbnailCache.h"

#include <QFileInfo>
#include <QLocale>

namespace fv {

GalleryModel::GalleryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&ThumbnailCache::instance(), &ThumbnailCache::ready,
            this, &GalleryModel::thumbReady, Qt::QueuedConnection);
}

void GalleryModel::setEntries(const QVector<FileEntry> &entries, bool timelineMode)
{
    beginResetModel();
    m_entries = entries;
    m_thumbs.clear();
    m_timeline = timelineMode;
    if (!entries.isEmpty())
        m_folder = QFileInfo(entries.first().path).absolutePath();
    endResetModel();

    // Kick off thumbnail loading for visible range; delegates call data()
    // which triggers the request on demand (lazy).
}

void GalleryModel::clear()
{
    beginResetModel();
    m_entries.clear();
    m_thumbs.clear();
    m_folder.clear();
    endResetModel();
}

QString GalleryModel::filePathAt(const QModelIndex &index) const
{
    return index.isValid() && index.row() < m_entries.size()
            ? m_entries[index.row()].path : QString();
}

int GalleryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant GalleryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();
    const FileEntry &e = m_entries[index.row()];
    const QFileInfo fi(e.path);

    switch (role) {
    case FilePathRole:  return e.path;
    case FileNameRole:  return fi.fileName();
    case ModifiedRole:  return e.modified;
    case FileSizeRole:  return double(e.size);
    case SectionRole:
        if (!m_timeline)
            return QString();
        return QLocale().toString(e.modified.date(), QStringLiteral("yyyy MMMM"));
    case ThumbRole: {
        QImage cached = m_thumbs.value(e.path);
        if (cached.isNull()) {
            cached = ThumbnailCache::instance().cached(e.path, 320);
            if (!cached.isNull())
                const_cast<GalleryModel *>(this)->m_thumbs.insert(e.path, cached);
            else
                ThumbnailCache::instance().request(e.path, 320);   // lazy load
        }
        return cached;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> GalleryModel::roleNames() const
{
    return {
        { FilePathRole, "filePath" },
        { FileNameRole, "fileName" },
        { ThumbRole,    "thumb" },
        { SectionRole,  "section" },
        { FileSizeRole, "fileSize" },
        { ModifiedRole, "modified" }
    };
}

void GalleryModel::thumbReady(const QString &path, const QImage &thumb)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].path == path) {
            m_thumbs.insert(path, thumb);
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, { ThumbRole });
            return;
        }
    }
}

} // namespace fv
