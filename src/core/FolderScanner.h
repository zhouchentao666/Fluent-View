#pragma once

#include <QDateTime>
#include <QStringList>
#include <QVector>

namespace fv {

struct FileEntry {
    QString    path;
    qint64     size = 0;
    QDateTime  modified;
};

// Non-GUI helper that lists supported images in a folder.
class FolderScanner {
public:
    // Lists supported images in `dir` (fast, no recursion unless asked).
    static QVector<FileEntry> scan(const QString &dir, bool recursive = false);

    // Natural sort ("img2" < "img10"); dirs are excluded by scan().
    static void naturalSort(QVector<FileEntry> &entries);

    // Recently used folders (deduplicated, from Settings persistence helper).
    static QStringList commonFolders();
};

} // namespace fv
