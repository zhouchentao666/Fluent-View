#include "FolderScanner.h"

#include "FormatRegistry.h"

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace fv {

namespace {

// Case-insensitive natural comparison with digit runs compared numerically
// ("img2" < "img10").
int naturalCompare(const QString &a, const QString &b)
{
    static QCollator collator = [] {
        QCollator c;
        c.setNumericMode(true);
        c.setCaseSensitivity(Qt::CaseInsensitive);
        return c;
    }();
    return collator.compare(a, b);
}

struct NaturalLess {
    bool operator()(const FileEntry &a, const FileEntry &b) const
    {
        const QFileInfo fa(a.path);
        const QFileInfo fb(b.path);
        // Sort by pure numeric-aware comparison of the file name.
        const int c = naturalCompare(fa.fileName(), fb.fileName());
        if (c != 0) return c < 0;
        return naturalCompare(fa.absolutePath(), fb.absolutePath()) < 0;
    }
};

} // namespace

QVector<FileEntry> FolderScanner::scan(const QString &dir, bool recursive)
{
    QVector<FileEntry> entries;
    const QStringList &suffixes = FormatRegistry::instance().supportedSuffixes();
    QStringList nameFilters;
    nameFilters.reserve(suffixes.size());
    for (const QString &s : suffixes)
        nameFilters.append(QStringLiteral("*.%1").arg(s));

    QDir::Filters filters = QDir::Files | QDir::Readable;

    if (recursive) {
        QDirIterator it(dir, nameFilters, filters,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            const QFileInfo fi(it.nextFileInfo());
            entries.append({ it.filePath(), fi.size(), fi.lastModified() });
        }
    } else {
        const QFileInfoList list =
            QDir(dir).entryInfoList(nameFilters, filters, QDir::NoSort);
        entries.reserve(list.size());
        for (const QFileInfo &fi : list)
            entries.append({ fi.absoluteFilePath(), fi.size(), fi.lastModified() });
    }

    naturalSort(entries);
    return entries;
}

void FolderScanner::naturalSort(QVector<FileEntry> &entries)
{
    std::sort(entries.begin(), entries.end(), NaturalLess());
}

QStringList FolderScanner::commonFolders()
{
    QStringList folders;
    folders.append(QDir::homePath() + QStringLiteral("/Pictures"));
    folders.append(QDir::homePath() + QStringLiteral("/Downloads"));
    folders.append(QDir::homePath() + QStringLiteral("/Desktop"));
    QStringList result;
    for (const QString &f : folders)
        if (QFileInfo::exists(f))
            result.append(QDir(f).absolutePath());
    return result;
}

} // namespace fv
