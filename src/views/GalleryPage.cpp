#include "views/GalleryPage.h"

#include "app/Settings.h"
#include "core/FolderScanner.h"
#include "core/ThumbnailCache.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

#include <QtConcurrent>

namespace fv {

namespace {
constexpr int kThumbEdge = 320;
constexpr int kCellWidth = 180;
constexpr int kCellHeight = 200;
constexpr int kGridGap = 8;
}

// Paints one gallery cell: rounded thumbnail + file name underneath.
class GalleryDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);

        const QRect cell = option.rect;
        const QRect thumbRect = cell.adjusted(0, 0, 0, -24);

        // selection / hover background
        if (option.state & QStyle::State_Selected) {
            p->setPen(Qt::NoPen);
            p->setBrush(option.palette.color(QPalette::Highlight));
            p->drawRoundedRect(cell.adjusted(2, 2, -2, -2), 10, 10);
        } else if (option.state & QStyle::State_MouseOver) {
            QColor hover = option.palette.color(QPalette::Highlight);
            hover.setAlpha(60);
            p->setPen(Qt::NoPen);
            p->setBrush(hover);
            p->drawRoundedRect(cell.adjusted(2, 2, -2, -2), 10, 10);
        }

        const QImage thumb = index.data(GalleryModel::ThumbRole).value<QImage>();
        p->setPen(Qt::NoPen);
        p->setBrush(option.palette.color(QPalette::AlternateBase));
        p->drawRoundedRect(thumbRect, 8, 8);

        if (!thumb.isNull()) {
            QImage scaled = thumb.scaled(thumbRect.size(), Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
            QRect imgRect(QPoint(thumbRect.x() + (thumbRect.width() - scaled.width()) / 2,
                                 thumbRect.y() + (thumbRect.height() - scaled.height()) / 2),
                          scaled.size());
            QPainterPath clip;
            clip.addRoundedRect(thumbRect, 8, 8);
            p->setClipPath(clip);
            p->drawImage(imgRect, scaled);
            p->setClipping(false);
        }

        p->setPen(option.palette.color(QPalette::WindowText));
        const QString name = option.fontMetrics.elidedText(
            index.data(GalleryModel::FileNameRole).toString(), Qt::ElideMiddle, cell.width() - 8);
        p->drawText(QRect(cell.x(), thumbRect.bottom() + 4, cell.width(), 18),
                    Qt::AlignHCenter | Qt::AlignTop, name);
        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(kCellWidth, kCellHeight);
    }
};

GalleryPage::GalleryPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    setAcceptDrops(true);

    m_model = new GalleryModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_grid->setModel(m_proxy);
    connect(m_grid, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &proxyIdx) {
        const QModelIndex idx = m_proxy->mapToSource(proxyIdx);
        const QString path = m_model->filePathAt(idx);
        if (!path.isEmpty())
            emit fileRequested(path);
    });

    connect(&ThumbnailCache::instance(), &ThumbnailCache::ready, this, [this]() {
        // repaint visible cells as thumbs stream in
        m_grid->viewport()->update();
    });
}

void GalleryPage::buildUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- left: folder strip -------------------------------------------
    auto *side = new QWidget(this);
    side->setObjectName(QStringLiteral("gallerySide"));
    side->setFixedWidth(220);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(12, 16, 12, 12);

    auto *sideTitle = new QLabel(tr("Folders"), side);
    sideTitle->setObjectName(QStringLiteral("panelTitle"));
    sideLayout->addWidget(sideTitle);

    m_folderList = new QListWidget(side);
    m_folderList->setObjectName(QStringLiteral("folderList"));
    m_folderList->setFrameShape(QFrame::NoFrame);
    m_folderList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sideLayout->addWidget(m_folderList, 1);

    connect(m_folderList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *cur, QListWidgetItem *) {
        if (!cur) return;
        const QString dir = cur->data(Qt::UserRole).toString();
        if (!dir.isEmpty() && dir != currentFolder())
            openFolder(dir);
    });

    auto *openBtn = new QPushButton(tr("Open Folder..."), side);
    openBtn->setObjectName(QStringLiteral("accentButton"));
    sideLayout->addWidget(openBtn);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Choose a picture folder"), QDir::homePath());
        if (!dir.isEmpty())
            openFolder(dir);
    });

    root->addWidget(side);

    // ---- right: grid ----------------------------------------------------
    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 16, 20, 12);

    auto *headerRow = new QHBoxLayout();
    m_header = new QLabel(tr("Recent Pictures"), content);
    m_header->setObjectName(QStringLiteral("galleryHeader"));
    headerRow->addWidget(m_header);
    headerRow->addStretch(1);

    m_recursiveBtn = new QPushButton(tr("Include subfolders"), content);
    m_recursiveBtn->setCheckable(true);
    m_recursiveBtn->setChecked(Settings::instance().galleryRecursive());
    connect(m_recursiveBtn, &QPushButton::toggled, this, [this](bool on) {
        Settings::instance().setGalleryRecursive(on);
        if (!m_pendingFolder.isEmpty())
            startScan(m_pendingFolder);
    });
    headerRow->addWidget(m_recursiveBtn);

    m_toggleTimeline = new QPushButton(tr("Timeline"), content);
    m_toggleTimeline->setCheckable(true);
    connect(m_toggleTimeline, &QPushButton::toggled, this, [this](bool on) {
        m_timeline = on;
        m_toggleTimeline->setText(on ? tr("Grid") : tr("Timeline"));
        if (!m_pendingFolder.isEmpty())
            startScan(m_pendingFolder);
    });
    headerRow->addWidget(m_toggleTimeline);

    contentLayout->addLayout(headerRow);

    m_grid = new QListView(content);
    m_grid->setObjectName(QStringLiteral("galleryGrid"));
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Static);
    m_grid->setWrapping(true);
    m_grid->setUniformItemSizes(true);
    m_grid->setGridSize(QSize(kCellWidth, kCellHeight));
    m_grid->setSpacing(kGridGap);
    m_grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_grid->setFrameShape(QFrame::NoFrame);
    m_grid->setItemDelegate(new GalleryDelegate(m_grid));

    contentLayout->addWidget(m_grid, 1);
    root->addWidget(content, 1);
}

void GalleryPage::openFolder(const QString &dir)
{
    m_pendingFolder = dir;
    Settings::instance().addRecentFolder(dir);
    refreshRecents();
    startScan(dir);
}

void GalleryPage::refreshRecents()
{
    const QString keep = currentFolder();
    m_folderList->blockSignals(true);
    m_folderList->clear();
    QStringList folders = Settings::instance().recentFolders();
    if (folders.isEmpty())
        folders = FolderScanner::commonFolders();
    for (const QString &f : folders) {
        auto *item = new QListWidgetItem(
            QDir(f).dirName().isEmpty() ? f : QDir(f).dirName(), m_folderList);
        item->setData(Qt::UserRole, f);
        item->setToolTip(f);
    }
    m_folderList->blockSignals(false);
}

void GalleryPage::startScan(const QString &folder)
{
    const bool recursive = Settings::instance().galleryRecursive();
    const bool timeline = m_timeline;
    auto *watcher = new QFutureWatcher<QVector<FileEntry>>(this);
    connect(watcher, &QFutureWatcher<QVector<FileEntry>>::finished, this,
            [this, watcher, folder]() {
        onScanFinished(folder, watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([folder, recursive, timeline]() {
        QVector<FileEntry> entries = FolderScanner::scan(folder, recursive);
        if (timeline)
            std::sort(entries.begin(), entries.end(), [](const FileEntry &a, const FileEntry &b) {
                return a.modified > b.modified;
            });
        return entries;
    }));
}

void GalleryPage::onScanFinished(const QString &folder, const QVector<FileEntry> &entries)
{
    if (folder != m_pendingFolder)
        return;   // user moved on already
    m_header->setText(QDir(folder).absolutePath() + QStringLiteral("  -  %1")
                      .arg(tr("%n picture(s)", "", entries.size())));
    m_model->setEntries(entries, m_timeline);
    m_grid->scrollToTop();
}

void GalleryPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void GalleryPage::dropEvent(QDropEvent *event)
{
    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir()) {
            emit folderDropped(path);
        } else {
            files.append(path);
        }
    }
    if (!files.isEmpty())
        emit dropPaths(files);
    event->acceptProposedAction();
}

} // namespace fv
