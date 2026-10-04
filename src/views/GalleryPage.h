#pragma once

#include "views/GalleryModel.h"

#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QWidget>

namespace fv {

// Startup page when no file was given: a thumbnail wall of a folder plus a
// strip of recent/common folders. Drag&drop adds folders.
class GalleryPage : public QWidget {
    Q_OBJECT
public:
    explicit GalleryPage(QWidget *parent = nullptr);

    void openFolder(const QString &dir);       // scan + display
    void refreshRecents();
    bool isTimelineMode() const { return m_timeline; }
    QString currentFolder() const { return m_model->folder(); }

signals:
    void fileRequested(const QString &path);   // open in viewer
    void folderDropped(const QString &dir);
    void dropPaths(const QStringList &paths);  // forwarded to MainWindow

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onScanFinished(const QString &folder, const QVector<fv::FileEntry> &entries);

private:
    void buildUi();
    void startScan(const QString &folder);

    QLabel *m_header = nullptr;
    QListWidget *m_folderList = nullptr;
    QListView *m_grid = nullptr;
    QPushButton *m_toggleTimeline = nullptr;
    QPushButton *m_recursiveBtn = nullptr;
    GalleryModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
    QString m_pendingFolder;
    bool m_timeline = false;
};

} // namespace fv
