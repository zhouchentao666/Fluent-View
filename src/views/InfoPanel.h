#pragma once

#include "core/ImageTypes.h"

#include <QFormLayout>
#include <QLabel>
#include <QWidget>

namespace fv {

// Right-side collapsible panel showing EXIF and file facts.
class InfoPanel : public QWidget {
    Q_OBJECT
public:
    explicit InfoPanel(QWidget *parent = nullptr);

    void setMaxWidth(int w) { setFixedWidth(w); }

public slots:
    void showInfo(const fv::LoadResult &result);
    void showError(const QString &fileName, const QString &message);

private:
    void addRow(const QString &key, const QString &value);

    QLabel *m_fileName = nullptr;
    QFormLayout *m_form = nullptr;
    QList<QLabel *> m_valueLabels;
};

} // namespace fv
