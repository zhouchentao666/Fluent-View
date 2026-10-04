#include "views/InfoPanel.h"

#include <QDir>
#include <QFileInfo>
#include <QScrollArea>
#include <QVBoxLayout>

namespace fv {

InfoPanel::InfoPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("infoPanel"));
    setFixedWidth(280);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scroll);
    auto *layout = new QVBoxLayout(content);

    m_fileName = new QLabel(content);
    m_fileName->setObjectName(QStringLiteral("infoPanelFileName"));
    m_fileName->setWordWrap(true);
    m_fileName->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_fileName);

    auto *formHost = new QWidget(content);
    m_form = new QFormLayout(formHost);
    m_form->setLabelAlignment(Qt::AlignRight);
    m_form->setHorizontalSpacing(12);
    m_form->setVerticalSpacing(6);
    layout->addWidget(formHost);
    layout->addStretch(1);

    scroll->setWidget(content);
    outer->addWidget(scroll);
}

void InfoPanel::addRow(const QString &key, const QString &value)
{
    if (value.isEmpty())
        return;
    auto *v = new QLabel(value, this);
    v->setWordWrap(true);
    v->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->setToolTip(value);
    m_form->addRow(new QLabel(key, this), v);
    m_valueLabels.append(v);
}

void InfoPanel::showInfo(const LoadResult &result)
{
    // clear
    while (m_form->rowCount() > 0) {
        m_form->removeRow(0);
    }
    m_valueLabels.clear();

    const QFileInfo fi(result.path);
    m_fileName->setText(fi.fileName());

    // file facts
    addRow(tr("Path"), QDir::toNativeSeparators(fi.absolutePath()));
    addRow(tr("Size"), double(fi.size()) > 1024 * 1024
            ? QStringLiteral("%1 MB").arg(fi.size() / 1024.0 / 1024.0, 0, 'f', 2)
            : QStringLiteral("%1 KB").arg(fi.size() / 1024.0, 0, 'f', 1));
    addRow(tr("Type"), fi.suffix().toUpper());
    addRow(tr("Modified"), fi.lastModified().toString(Qt::SystemLocaleShortDate));
    addRow(tr("Dimensions"),
           QStringLiteral("%1 x %2 px").arg(result.image.width()).arg(result.image.height()));
    if (!result.decoder.isEmpty())
        addRow(tr("Decoder"), result.decoder);
    if (!result.layers.isEmpty())
        addRow(tr("Layers"), QString::number(result.layers.size()));

    const fv::ExifData &ex = result.exif;
    if (ex.valid) {
        QString camera = ex.cameraMake;
        if (!ex.cameraModel.isEmpty())
            camera += (camera.isEmpty() ? QString() : QLatin1Char(' ')) + ex.cameraModel;
        addRow(tr("Camera"), camera);
        addRow(tr("Lens"), ex.lensModel);
        addRow(tr("Exposure time"), fv::formatExposure(ex.exposureTime));
        addRow(tr("F-number"), ex.fNumber > 0 ? QStringLiteral("f/%1").arg(ex.fNumber) : QString());
        addRow(tr("ISO"), ex.iso > 0 ? QString::number(ex.iso) : QString());
        addRow(tr("Focal length"),
               ex.focalLength > 0 ? QStringLiteral("%1 mm").arg(ex.focalLength) : QString());
        addRow(tr("Date taken"), ex.dateTime);
        addRow(tr("Artist"), ex.artist);
        addRow(tr("Software"), ex.software);
        if (!qIsNaN(ex.gpsLatitude) && !qIsNaN(ex.gpsLongitude)) {
            addRow(tr("GPS"), QStringLiteral("%1, %2")
                       .arg(ex.gpsLatitude, 0, 'f', 6)
                       .arg(ex.gpsLongitude, 0, 'f', 6));
        }
        if (!qIsNaN(ex.gpsAltitude))
            addRow(tr("Altitude"), QStringLiteral("%1 m").arg(ex.gpsAltitude, 0, 'f', 0));
    }

    setVisible(true);
}

void InfoPanel::showError(const QString &fileName, const QString &message)
{
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);
    m_valueLabels.clear();
    m_fileName->setText(fileName);
    addRow(tr("Error"), message);
    setVisible(true);
}

} // namespace fv
