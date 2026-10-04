#include "views/TitleBar.h"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QWindow>

namespace fv {

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(40);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(12, 0, 0, 0);
    m_layout->setSpacing(4);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("titleBarTitle"));
    m_layout->addWidget(m_title);
    m_layout->addStretch(1);

#ifdef Q_OS_MACOS
    // macOS: native traffic lights remain visible on frameless-with-
    // fullscreen-button windows; we only provide the drag/title surface.
#else
    setupWindowsControls();
#endif
}

void TitleBar::setupWindowsControls()
{
    m_controls = new QWidget(this);
    auto *controlsLayout = new QHBoxLayout(m_controls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(0);

    auto makeButton = [this](const char *objectName, const char *icon, const char *tip) {
        QPushButton *b = new QPushButton(m_controls);
        b->setObjectName(QLatin1String(objectName));
        b->setIcon(QIcon(QLatin1String(icon)));
        b->setToolTip(QLatin1String(tip));
        b->setFixedSize(46, 40);
        b->setFlat(true);
        return b;
    };

    QPushButton *minB = makeButton("titleBarMin", ":/icons/light/minimize.svg",
                                   tr("Minimize"));
    QPushButton *maxB = makeButton("titleBarMax", ":/icons/light/maximize.svg",
                                   tr("Maximize"));
    QPushButton *closeB = makeButton("titleBarClose", ":/icons/light/close.svg",
                                     tr("Close"));

    controlsLayout->addWidget(minB);
    controlsLayout->addWidget(maxB);
    controlsLayout->addWidget(closeB);
    m_layout->addWidget(m_controls);

    connect(minB, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(maxB, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    connect(closeB, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitle(const QString &title)
{
    m_title->setText(title);
}

void TitleBar::setDark(bool dark)
{
    Q_UNUSED(dark);   // handled via QSS palette
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (window()->windowHandle())
            window()->windowHandle()->startSystemMove();
        m_dragging = true;
        m_dragStart = event->globalPosition().toPoint();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    // Handled by startSystemMove.
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit maximizeClicked();
}

} // namespace fv
