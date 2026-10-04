#include "views/FloatingToolbar.h"

#include "app/ThemeManager.h"

#include <QHBoxLayout>

namespace fv {

FloatingToolbar::FloatingToolbar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("floatingToolbar"));
    setFixedHeight(48);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(2);

    auto makeButton = [this](const QString &iconName, const QString &tooltip) {
        auto *b = new QPushButton(this);
        b->setIcon(ThemeManager::instance().icon(iconName));
        b->setToolTip(tooltip);
        b->setFixedSize(36, 36);
        b->setFlat(true);
        b->setCursor(Qt::PointingHandCursor);
        layout->addWidget(b);
        return b;
    };

    m_thumbs      = makeButton(QStringLiteral("thumbnails"), tr("Toggle thumbnail bar"));
    layout->addSpacing(6);
    m_zoomOut     = makeButton(QStringLiteral("zoom-out"), tr("Zoom out (-)"));
    m_zoomLabel   = new QLabel(this);
    m_zoomLabel->setObjectName(QStringLiteral("toolbarZoomLabel"));
    m_zoomLabel->setMinimumWidth(56);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_zoomLabel);
    m_zoomIn      = makeButton(QStringLiteral("zoom-in"), tr("Zoom in (+)"));
    layout->addSpacing(6);
    m_fit         = makeButton(QStringLiteral("fit"), tr("Fit window (Ctrl+F)"));
    m_actual      = makeButton(QStringLiteral("actual-size"), tr("Actual size (Ctrl+1)"));
    layout->addSpacing(6);
    m_rotateLeft  = makeButton(QStringLiteral("rotate-left"), tr("Rotate left (Ctrl+,)"));
    m_rotateRight = makeButton(QStringLiteral("rotate-right"), tr("Rotate right (Ctrl+.)"));
    m_flipH       = makeButton(QStringLiteral("flip-h"), tr("Flip horizontal"));
    m_flipV       = makeButton(QStringLiteral("flip-v"), tr("Flip vertical"));
    layout->addSpacing(6);
    m_slideshow   = makeButton(QStringLiteral("slideshow"), tr("Slideshow (F5)"));
    m_info        = makeButton(QStringLiteral("info"), tr("Toggle info panel (Ctrl+I)"));
    layout->addSpacing(6);
    m_menu        = makeButton(QStringLiteral("menu"), tr("Menu"));

    layout->addStretch(1);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName(QStringLiteral("toolbarInfoLabel"));
    layout->addWidget(m_infoLabel);
}

QPushButton *FloatingToolbar::add(const QString &iconName, const QString &tooltip)
{
    auto *b = new QPushButton(this);
    b->setIcon(ThemeManager::instance().icon(iconName));
    b->setToolTip(tooltip);
    b->setFixedSize(36, 36);
    b->setFlat(true);
    b->setCursor(Qt::PointingHandCursor);
    layout()->addWidget(b);
    return b;
}

void FloatingToolbar::setZoomText(const QString &text)
{
    m_zoomLabel->setText(text);
}

void FloatingToolbar::setInfoText(const QString &text)
{
    m_infoLabel->setText(text);
}

} // namespace fv
