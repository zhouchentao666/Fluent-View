#include "views/MainWindow.h"

#include "platform/PlatformIntegration.h"
#include "app/Settings.h"
#include "app/ThemeManager.h"
#include "core/FolderScanner.h"
#include "core/ImageLoader.h"
#include "views/FloatingToolbar.h"
#include "views/InfoPanel.h"
#include "views/ThumbnailBar.h"
#include "views/TitleBar.h"

#include "core/FormatRegistry.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPrinter>
#include <QPrintDialog>
#include <QShortcut>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <QtConcurrent>

namespace fv {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(QStringLiteral("Fluent-View"));
    setAcceptDrops(true);
    setMinimumSize(QSize(720, 480));

    buildUi();
    buildShortcuts();
    applyChrome();

    m_slideshowTimer.setInterval(Settings::instance().slideshowIntervalMs());
    connect(&m_slideshowTimer, &QTimer::timeout, this, [this]() {
        if (m_files.isEmpty())
            return;
        int next = m_index + 1;
        if (next >= m_files.size())
            next = 0;
        loadIndex(next);
    });

    m_toolbarAutoHide.setSingleShot(true);
    m_toolbarAutoHide.setInterval(2400);
    connect(&m_toolbarAutoHide, &QTimer::timeout, this, [this]() {
        if (isFullScreen() && m_toolbar)
            m_toolbar->hide();
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this](bool) { applyChrome(); });

    const QByteArray geo = Settings::instance().mainWindowGeometry();
    if (!geo.isEmpty())
        restoreGeometry(geo);
    else
        resize(1100, 720);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(1, 1, 1, 1);
    root->setSpacing(0);

    m_titleBar = new TitleBar(central);
    connect(m_titleBar, SIGNAL(minimizeClicked()), this, SLOT(showMinimized()));
    connect(m_titleBar, &TitleBar::maximizeClicked, this, [this]() {
        if (isMaximized()) showNormal(); else showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeClicked, this, &MainWindow::close);
    root->addWidget(m_titleBar);

    auto *bodyRow = new QHBoxLayout();
    bodyRow->setContentsMargins(0, 0, 0, 0);
    bodyRow->setSpacing(0);

    m_thumbs = new ThumbnailBar(central);
    m_thumbs->setVisible(Settings::instance().showThumbnailBar());
    connect(m_thumbs, &ThumbnailBar::fileActivated, this, &MainWindow::openFileAt);
    bodyRow->addWidget(m_thumbs);

    m_stack = new QStackedWidget(central);
    m_gallery = new GalleryPage(m_stack);
    m_stack->addWidget(m_gallery);

    m_view = new ImageView(m_stack);
    m_stack->addWidget(m_view);

    connect(m_gallery, &GalleryPage::fileRequested, this, &MainWindow::openFileAt);
    connect(m_gallery, &GalleryPage::dropPaths, this, &MainWindow::openPaths);
    connect(m_gallery, &GalleryPage::folderDropped, m_gallery, &GalleryPage::openFolder);

    bodyRow->addWidget(m_stack, 1);

    m_info = new InfoPanel(central);
    m_info->setVisible(Settings::instance().showInfoPanel());
    bodyRow->addWidget(m_info);

    root->addLayout(bodyRow, 1);

    m_toolbar = new FloatingToolbar(central);
    root->addWidget(m_toolbar);
    root->setAlignment(m_toolbar, Qt::AlignHCenter);

    setCentralWidget(central);

    // toolbar actions
    connect(m_toolbar->btnZoomIn(),  &QPushButton::clicked, m_view, &ImageView::zoomIn);
    connect(m_toolbar->btnZoomOut(), &QPushButton::clicked, m_view, &ImageView::zoomOut);
    connect(m_toolbar->btnFit(),     &QPushButton::clicked, m_view, &ImageView::fitWindow);
    connect(m_toolbar->btnActual(),  &QPushButton::clicked, m_view, &ImageView::actualSize);
    connect(m_toolbar->btnRotateLeft(),  &QPushButton::clicked, m_view, &ImageView::rotateLeft);
    connect(m_toolbar->btnRotateRight(), &QPushButton::clicked, m_view, &ImageView::rotateRight);
    connect(m_toolbar->btnFlipH(), &QPushButton::clicked, m_view, &ImageView::flipHorizontal);
    connect(m_toolbar->btnFlipV(), &QPushButton::clicked, m_view, &ImageView::flipVertical);
    connect(m_toolbar->btnSlideshow(), &QPushButton::clicked, this, &MainWindow::startSlideshow);
    connect(m_toolbar->btnInfo(), &QPushButton::clicked, this, [this]() {
        const bool on = !m_info->isVisible();
        m_info->setVisible(on);
        Settings::instance().setShowInfoPanel(on);
    });
    connect(m_toolbar->btnThumbs(), &QPushButton::clicked, this, [this]() {
        const bool on = !m_thumbs->isVisible();
        m_thumbs->setVisible(on);
        Settings::instance().setShowThumbnailBar(on);
    });

    // context menu on the toolbar's menu button
    auto menuButton = m_toolbar->btnMenu();
    connect(menuButton, &QPushButton::clicked, this, [this, menuButton]() {
        QMenu menu(this);
        QAction *openFile = menu.addAction(tr("Open File..."));
        QAction *openDir = menu.addAction(tr("Open Folder..."));
        menu.addSeparator();
        QAction *copyAct = menu.addAction(tr("Copy to clipboard"));
        QAction *wallpaperAct = menu.addAction(tr("Set as wallpaper"));
        QAction *printAct = menu.addAction(tr("Print..."));
        menu.addSeparator();
        QAction *fullAct = menu.addAction(tr("Fullscreen (F11)"));
        menu.addSeparator();
        QMenu *themeMenu = menu.addMenu(tr("Theme"));
        QActionGroup *themeGroup = new QActionGroup(&menu);
        auto addTheme = [&](const QString &id, const QString &label) {
            QAction *a = themeMenu->addAction(label);
            a->setCheckable(true);
            a->setChecked(ThemeManager::instance().preference() == id);
            themeGroup->addAction(a);
            connect(a, &QAction::triggered, this, [id]() {
                ThemeManager::instance().setPreference(id);
            });
        };
        addTheme(QStringLiteral("system"), tr("Follow system"));
        addTheme(QStringLiteral("light"), tr("Light"));
        addTheme(QStringLiteral("dark"), tr("Dark"));
        menu.addSeparator();
        QAction *defaultAct = menu.addAction(tr("Set as default viewer..."));

        QAction *picked = menu.exec(menuButton->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
        if (picked == openFile) {
            const QStringList files = QFileDialog::getOpenFileNames(
                this, tr("Open images"), QString(),
                FormatRegistry::instance().openFileFilter() + QStringLiteral(";;All files (*)"));
            if (!files.isEmpty()) openPaths(files);
        } else if (picked == openDir) {
            const QString dir = QFileDialog::getExistingDirectory(this, tr("Open folder"));
            if (!dir.isEmpty()) {
                m_stack->setCurrentWidget(m_gallery);
                m_gallery->openFolder(dir);
            }
        } else if (picked == copyAct) {
            QApplication::clipboard()->setImage(m_view->grab().toImage());
        } else if (picked == wallpaperAct) {
            PlatformIntegration::setWallpaper(m_files.value(m_index).path);
        } else if (picked == printAct) {
            QPrinter printer(QPrinter::HighResolution);
            QPrintDialog dlg(&printer, this);
            if (dlg.exec() == QDialog::Accepted) {
                QPainter painter(&printer);
                const QRect page = painter.viewport();
                QImage img = m_view->grab().toImage();
                if (!img.isNull())
                    painter.drawImage(page, img);
            }
        } else if (picked == fullAct) {
            toggleFullscreen();
        } else if (picked == defaultAct) {
            PlatformIntegration::registerAsDefaultViewer(this);
        }
    });
}

void MainWindow::buildShortcuts()
{
    auto *next = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(next, &QShortcut::activated, this, [this]() { if (m_index >= 0) loadIndex((m_index + 1) % m_files.size()); });

    auto *prev = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(prev, &QShortcut::activated, this, [this]() {
        if (m_index >= 0) loadIndex((m_index - 1 + m_files.size()) % m_files.size());
    });

    auto *home = new QShortcut(QKeySequence::MoveToStartOfDocument, this);
    connect(home, &QShortcut::activated, this, [this]() { if (!m_files.isEmpty()) loadIndex(0); });

    auto *end = new QShortcut(QKeySequence::MoveToEndOfDocument, this);
    connect(end, &QShortcut::activated, this, [this]() { if (!m_files.isEmpty()) loadIndex(m_files.size() - 1); });

    auto *plus = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    connect(plus, &QShortcut::activated, m_view, &ImageView::zoomIn);
    auto *minus = new QShortcut(QKeySequence(Qt::Key_Minus), this);
    connect(minus, &QShortcut::activated, m_view, &ImageView::zoomOut);

    auto *fit = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(fit, &QShortcut::activated, m_view, &ImageView::fitWindow);
    auto *one = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(one, &QShortcut::activated, m_view, &ImageView::actualSize);

    auto *rotL = new QShortcut(QKeySequence("Ctrl+,"), this);
    connect(rotL, &QShortcut::activated, m_view, &ImageView::rotateLeft);
    auto *rotR = new QShortcut(QKeySequence("Ctrl+."), this);
    connect(rotR, &QShortcut::activated, m_view, &ImageView::rotateRight);

    auto *info = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(info, &QShortcut::activated, this, [this]() {
        const bool on = !m_info->isVisible();
        m_info->setVisible(on);
        Settings::instance().setShowInfoPanel(on);
    });
    auto *thumbToggle = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(thumbToggle, &QShortcut::activated, this, [this]() {
        const bool on = !m_thumbs->isVisible();
        m_thumbs->setVisible(on);
        Settings::instance().setShowThumbnailBar(on);
    });

    auto *gallery = new QShortcut(QKeySequence("Ctrl+G"), this);
    connect(gallery, &QShortcut::activated, this, [this]() {
        m_stack->setCurrentWidget(m_gallery);
    });

    auto *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(esc, &QShortcut::activated, this, [this]() {
        if (m_slideshowActive) stopSlideshow();
        else if (isFullScreen()) toggleFullscreen();
        else if (m_stack->currentWidget() == m_view) m_stack->setCurrentWidget(m_gallery);
    });

    auto *full = new QShortcut(QKeySequence("F11"), this);
    connect(full, &QShortcut::activated, this, &MainWindow::toggleFullscreen);

    auto *slide = new QShortcut(QKeySequence("F5"), this);
    connect(slide, &QShortcut::activated, this, [this]() {
        if (m_slideshowActive) stopSlideshow(); else startSlideshow();
    });

    auto *copy = new QShortcut(QKeySequence("Ctrl+C"), this);
    connect(copy, &QShortcut::activated, this, [this]() {
        if (m_index >= 0)
            QApplication::clipboard()->setImage(m_view->grab().toImage());
    });

    auto *del = new QShortcut(QKeySequence("Ctrl+O"), this);
    connect(del, &QShortcut::activated, this, [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(
            this, tr("Open images"), QString(),
            FormatRegistry::instance().openFileFilter() + QStringLiteral(";;All files (*)"));
        if (!files.isEmpty()) openPaths(files);
    });

    auto *delDir = new QShortcut(QKeySequence("Ctrl+Shift+O"), this);
    connect(delDir, &QShortcut::activated, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Open folder"));
        if (!dir.isEmpty()) {
            m_stack->setCurrentWidget(m_gallery);
            m_gallery->openFolder(dir);
        }
    });

    connect(m_view, &ImageView::doubleClicked, this, &MainWindow::toggleFullscreen);
    connect(m_view, &ImageView::zoomChanged, this, [this](qreal z) {
        m_toolbar->setZoomText(QString::number(qRound(z * 100)) + QStringLiteral("%"));
    });
    connect(m_view, &ImageView::sizeChanged, this, [this](const QSize &) { updateToolbarInfo(); });
}

// --------------------------------------------------------------------------
// Open / navigate
// --------------------------------------------------------------------------
void MainWindow::openPaths(const QStringList &paths)
{
    if (paths.isEmpty())
        return;

    // Multiple files: open them all as a playlist.
    if (paths.size() > 1) {
        QVector<FileEntry> entries;
        for (const QString &p : paths) {
            const QFileInfo fi(p);
            if (fi.isFile())
                entries.append({ fi.absoluteFilePath(), fi.size(), fi.lastModified() });
        }
        if (!entries.isEmpty()) {
            setFiles(entries, 0);
            return;
        }
    }

    const QString path = paths.first();
    if (QFileInfo(path).isDir()) {
        m_stack->setCurrentWidget(m_gallery);
        m_gallery->openFolder(path);
        return;
    }
    openFileAt(path);
}

void MainWindow::openFileAt(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists())
        return;

    const QString dir = fi.absolutePath();
    if (dir != m_singleFileDir || m_files.isEmpty()) {
        m_singleFileDir = dir;
        Settings::instance().addRecentFolder(dir);
        // Show the folder contents around the picked file.
        QPointer<MainWindow> guard(this);
        auto *watcher = new QFutureWatcher<QVector<FileEntry>>(this);
        connect(watcher, &QFutureWatcher<QVector<FileEntry>>::finished, this,
                [this, watcher, path, guard]() {
            if (!guard) return;
            const QVector<FileEntry> entries = watcher->result();
            int idx = -1;
            for (int i = 0; i < entries.size(); ++i) {
                if (entries[i].path == path) { idx = i; break; }
            }
            if (idx < 0) {
                // not in the folder listing (e.g. unsupported scan): single file
                QVector<FileEntry> single{ { path, QFileInfo(path).size(),
                                             QFileInfo(path).lastModified() } };
                setFiles(single, 0);
            } else {
                setFiles(entries, idx);
            }
            watcher->deleteLater();
        });
        watcher->setFuture(QtConcurrent::run([dir]() {
            return FolderScanner::scan(dir, false);
        }));
    } else {
        for (int i = 0; i < m_files.size(); ++i) {
            if (m_files[i].path == path) {
                loadIndex(i);
                return;
            }
        }
    }
}

void MainWindow::setFiles(const QVector<FileEntry> &files, int startIndex)
{
    if (files.isEmpty())
        return;
    m_files = files;
    m_thumbs->setFiles(files);
    m_stack->setCurrentWidget(m_view);
    loadIndex(qBound(0, startIndex, files.size() - 1));
}

void MainWindow::loadIndex(int index, bool animate)
{
    Q_UNUSED(animate);
    if (index < 0 || index >= m_files.size())
        return;
    m_index = index;
    const QString path = m_files[index].path;
    setWindowTitle(QStringLiteral("%1 - Fluent-View").arg(QFileInfo(path).fileName()));
    if (m_titleBar)
        static_cast<TitleBar *>(m_titleBar)->setTitle(QFileInfo(path).fileName());

    m_thumbs->setActive(path);
    updateToolbarInfo();

    // Async full decode
    disconnect(m_loadConn);
    m_loadConn = connect(&ImageLoader::instance(), &ImageLoader::loaded, this,
            [this](const QString &p, const LoadResult &result) {
        if (p != m_files.value(m_index).path)
            return;   // stale
        m_view->setImage(result);
        m_info->showInfo(result);
        m_toolbar->setInfoText(QStringLiteral("%1 x %2  -  %3")
                .arg(result.image.width())
                .arg(result.image.height())
                .arg(double(result.fileSize) / 1024 / 1024, 0, 'f', 2) + QStringLiteral(" MB"));
    });
    disconnect(m_failConn);
    m_failConn = connect(&ImageLoader::instance(), &ImageLoader::loadFailed, this,
            [this](const QString &p, const QString &error, const QStringList &rows, bool unknown) {
        if (p != m_files.value(m_index).path)
            return;
        m_view->showError(p, unknown
                ? tr("The format of this file is not supported in this build.")
                : error, rows);
        m_info->showError(QFileInfo(p).fileName(), error);
    });
    ImageLoader::instance().load(path);
}

void MainWindow::updateToolbarInfo()
{
    // Placeholder; the load callback keeps the label current.
}

void MainWindow::applyChrome()
{
    const bool custom = Settings::instance().customTitlebar();
#ifdef Q_OS_MACOS
    m_titleBar->setVisible(false);
    setWindowFlag(Qt::FramelessWindowHint, false);
    setUnifiedTitleAndToolBarOnMac(true);
#else
    m_titleBar->setVisible(custom);
    setWindowFlag(Qt::FramelessWindowHint, custom && !isFullScreen());
#endif
    // Re-show after flag change.
    if (isVisible())
        show();
}

void MainWindow::positionToolbar()
{
    if (!m_toolbar)
        return;
    const int x = (width() - m_toolbar->width()) / 2;
    const int y = height() - m_toolbar->height() - 16;
    m_toolbar->move(x, y);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionToolbar();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        // toggle chrome (toolbar) in fullscreen
        m_toolbar->setVisible(!m_toolbar->isVisible());
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            files.append(url.toLocalFile());
    }
    if (!files.isEmpty())
        openPaths(files);
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Settings::instance().setMainWindowGeometry(saveGeometry());
    QMainWindow::closeEvent(event);
}

// --------------------------------------------------------------------------
// Fullscreen & slideshow
// --------------------------------------------------------------------------
void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        m_toolbar->show();
        applyChrome();
    } else {
        applyChrome();
        showFullScreen();
        m_toolbar->show();
        m_toolbar->raise();
        positionToolbar();
        m_toolbarAutoHide.start();
    }
}

void MainWindow::startSlideshow()
{
    if (m_files.isEmpty())
        return;
    m_slideshowActive = true;
    m_slideshowTimer.start();
    toggleFullscreen();
}

void MainWindow::stopSlideshow()
{
    m_slideshowActive = false;
    m_slideshowTimer.stop();
    if (isFullScreen())
        toggleFullscreen();
}

} // namespace fv
