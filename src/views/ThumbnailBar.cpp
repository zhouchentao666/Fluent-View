#include "views/ThumbnailBar.h"

#include "core/ThumbnailCache.h"

#include <QFileInfo>
#include <QIcon>
#include <QScrollBar>

namespace fv {

namespace {
constexpr int kThumbEdge = 96;
constexpr int kIconSize = kThumbEdge + 16;
}

ThumbnailBar::ThumbnailBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("thumbnailBar"));
    setFixedWidth(kIconSize + 24);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("thumbnailList"));
    m_list->setViewMode(QListView::IconMode);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setWrapping(false);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setIconSize(QSize(kIconSize, kIconSize));
    m_list->setUniformItemSizes(true);
    m_list->setSpacing(8);
    m_list->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        const int row = m_list->row(item);
        if (row >= 0 && row < m_files.size())
            emit fileActivated(m_files[row].path);
    });
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        const int row = m_list->row(item);
        if (row >= 0 && row < m_files.size())
            emit fileActivated(m_files[row].path);
    });

    connect(&ThumbnailCache::instance(), &ThumbnailCache::ready, this,
            [this](const QString &path, const QImage &thumb) {
        for (int i = 0; i < m_files.size() && i < m_list->count(); ++i) {
            if (m_files[i].path == path) {
                QListWidgetItem *it = m_list->item(i);
                if (it && it->icon().isNull())
                    it->setIcon(QIcon(QPixmap::fromImage(thumb)));
                break;
            }
        }
    });
}

void ThumbnailBar::setFiles(const QVector<FileEntry> &files)
{
    m_files = files;
    m_list->clear();

    for (const FileEntry &e : files) {
        auto *item = new QListWidgetItem(QFileInfo(e.path).fileName(), m_list);
        item->setData(Qt::UserRole, e.path);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        item->setSizeHint(QSize(kIconSize, kIconSize + 20));
        ThumbnailCache::instance().request(e.path, kThumbEdge);
    }
}

void ThumbnailBar::setActive(const QString &path)
{
    for (int i = 0; i < m_files.size(); ++i) {
        if (m_files[i].path == path) {
            m_list->setCurrentRow(i);
            m_list->scrollToItem(m_list->item(i), QAbstractItemView::EnsureVisible);
            break;
        }
    }
}

} // namespace fv
