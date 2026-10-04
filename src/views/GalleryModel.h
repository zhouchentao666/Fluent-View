#pragma once

#include "core/FolderScanner.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QImage>
#include <QVector>

namespace fv {

// Model behind the gallery grid. Thumbnails are requested lazily from the
// cache and pushed in via dataChanged as they arrive.
class GalleryModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        ThumbRole,
        SectionRole,     // date string, e.g. "2026-10"
        FileSizeRole,
        ModifiedRole
    };

    explicit GalleryModel(QObject *parent = nullptr);

    void setEntries(const QVector<fv::FileEntry> &entries, bool timelineMode);
    void clear();
    QString filePathAt(const QModelIndex &index) const;
    QString folder() const { return m_folder; }
    int count() const { return m_entries.size(); }

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private slots:
    void thumbReady(const QString &path, const QImage &thumb);

private:
    QVector<fv::FileEntry> m_entries;
    QHash<QString, QImage> m_thumbs;
    QString m_folder;
    bool m_timeline = false;
};

} // namespace fv
