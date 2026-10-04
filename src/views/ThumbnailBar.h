#pragma once

#include "core/FolderScanner.h"

#include <QListWidget>
#include <QVector>
#include <QWidget>

namespace fv {

// Left collapsible thumbnail strip. Thumbnails load lazily through the
// ThumbnailCache; clicking an entry jumps the viewer to that file.
class ThumbnailBar : public QWidget {
    Q_OBJECT
public:
    explicit ThumbnailBar(QWidget *parent = nullptr);

    void setFiles(const QVector<fv::FileEntry> &files);
    void setActive(const QString &path);

signals:
    void fileActivated(const QString &path);

private:
    QListWidget *m_list = nullptr;
    QVector<fv::FileEntry> m_files;
};

} // namespace fv
