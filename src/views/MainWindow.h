#pragma once

#include "core/FolderScanner.h"
#include "views/GalleryPage.h"
#include "views/ImageView.h"

#include <QMetaObject>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QVector>

class QLabel;
class QPushButton;

namespace fv {

class FloatingToolbar;
class InfoPanel;
class ThumbnailBar;
class TitleBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void openPaths(const QStringList &paths);   // entry from CLI / DnD / singleton
    void toggleFullscreen();
    void startSlideshow();
    void stopSlideshow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void buildShortcuts();
    void setFiles(const QVector<fv::FileEntry> &files, int startIndex);
    void loadIndex(int index, bool animate = true);
    void openFileAt(const QString &path);
    void updateToolbarInfo();
    void applyChrome();             // frameless / titlebar visibility
    void positionToolbar();

    // viewer state
    QVector<FileEntry> m_files;
    int    m_index = -1;
    QString m_singleFileDir;

    // ui
    QStackedWidget  *m_stack = nullptr;
    GalleryPage     *m_gallery = nullptr;
    ImageView       *m_view = nullptr;
    ThumbnailBar    *m_thumbs = nullptr;
    InfoPanel       *m_info = nullptr;
    FloatingToolbar *m_toolbar = nullptr;
    QWidget         *m_titleBar = nullptr;

    QTimer m_slideshowTimer;
    QTimer m_toolbarAutoHide;
    bool   m_slideshowActive = false;

    QMetaObject::Connection m_loadConn;
    QMetaObject::Connection m_failConn;
};

} // namespace fv
