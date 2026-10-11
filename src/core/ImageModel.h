#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QDir>

/**
 * Holds the list of images currently loaded and tracks the current index so the
 * UI can navigate previous/next. Opening a single file expands the list to all
 * supported images in its folder (folder navigation); opening several explicit
 * files keeps exactly that list.
 */
class ImageModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY currentIndexChanged)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY currentIndexChanged)

public:
    enum Roles { PathRole = Qt::UserRole + 1, NameRole };

    explicit ImageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentIndex() const { return m_index; }
    void setCurrentIndex(int i);

    QString currentPath() const;
    int count() const { return m_files.size(); }

    Q_INVOKABLE void setFiles(const QStringList &files);
    Q_INVOKABLE void openPath(const QString &path);
    Q_INVOKABLE void setFolder(const QString &folder);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE bool hasNext() const;
    Q_INVOKABLE bool hasPrevious() const;

signals:
    void currentIndexChanged();
    void countChanged();

private:
    void loadFolder(const QString &folder, const QString &select = QString());

    QStringList m_files;
    int m_index = -1;
};
