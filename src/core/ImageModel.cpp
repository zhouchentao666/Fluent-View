#include "ImageModel.h"
#include "Settings.h"

#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <algorithm>

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {}

int ImageModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_files.size();
}

QVariant ImageModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size())
        return {};
    const QString &p = m_files.at(index.row());
    switch (role) {
    case PathRole: return p;
    case NameRole: return QFileInfo(p).fileName();
    }
    return {};
}

QHash<int, QByteArray> ImageModel::roleNames() const {
    return {{PathRole, "path"}, {NameRole, "name"}};
}

QString ImageModel::currentPath() const {
    if (m_index < 0 || m_index >= m_files.size())
        return {};
    return m_files.at(m_index);
}

void ImageModel::setCurrentIndex(int i) {
    if (i < 0 || i >= m_files.size() || i == m_index)
        return;
    m_index = i;
    emit currentIndexChanged();
}

void ImageModel::setFiles(const QStringList &files) {
    QStringList normalized;
    normalized.reserve(files.size());
    for (const QString &f : files) {
        const QUrl url(f);
        normalized.append(url.isLocalFile() ? url.toLocalFile() : f);
    }
    beginResetModel();
    m_files = normalized;
    m_index = m_files.isEmpty() ? -1 : 0;
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

void ImageModel::loadFolder(const QString &folder, const QString &select) {
    const QStringList exts = Settings::supportedExtensions();
    QDir dir(folder);
    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList files;
    for (const QString &e : entries) {
        const QString ext = QFileInfo(e).suffix().toLower();
        if (exts.contains(ext))
            files.append(dir.absoluteFilePath(e));
    }
    std::sort(files.begin(), files.end(), [](const QString &a, const QString &b) {
        return QFileInfo(a).fileName().toLower() < QFileInfo(b).fileName().toLower();
    });

    beginResetModel();
    m_files = files;
    m_index = -1;
    if (!select.isEmpty()) {
        m_index = m_files.indexOf(QFileInfo(select).absoluteFilePath());
        if (m_index < 0) m_index = 0;
    }
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

void ImageModel::openPath(const QString &path) {
    const QString local = QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path;
    const QFileInfo info(local);
    if (info.isDir()) {
        loadFolder(local);
        return;
    }
    // Single file: navigate its whole folder.
    loadFolder(info.absolutePath(), local);
}

void ImageModel::setFolder(const QString &folder) {
    loadFolder(folder);
}

void ImageModel::next() {
    if (hasNext())
        setCurrentIndex(m_index + 1);
}

void ImageModel::previous() {
    if (hasPrevious())
        setCurrentIndex(m_index - 1);
}

bool ImageModel::hasNext() const { return m_index >= 0 && m_index + 1 < m_files.size(); }
bool ImageModel::hasPrevious() const { return m_index > 0; }
