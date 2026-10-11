#pragma once

#include <QObject>
#include <QString>
#include <QSize>

/**
 * Holds the state of the image currently being viewed: its path, the decode
 * metadata (resolution / format / size) read cheaply without a full decode, and
 * the display transform (rotation + flips). The QML Image binds to `source`,
 * which encodes the transform into the provider URL so the off-thread decode
 * already reflects rotation/flips.
 */
class ImageController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString source READ source NOTIFY displayChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY infoChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY infoChanged)
    Q_PROPERTY(QString format READ format NOTIFY infoChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY infoChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY infoChanged)
    Q_PROPERTY(int rotation READ rotation NOTIFY transformChanged)
    Q_PROPERTY(bool flipH READ flipH NOTIFY transformChanged)
    Q_PROPERTY(bool flipV READ flipV NOTIFY transformChanged)

public:
    explicit ImageController(QObject *parent = nullptr);

    QString path() const { return m_path; }
    void setPath(const QString &p);

    QString source() const;
    int imageWidth() const { return m_imgW; }
    int imageHeight() const { return m_imgH; }
    QString format() const { return m_format; }
    qint64 fileSize() const { return m_fileSize; }
    QString fileName() const { return m_fileName; }

    int rotation() const { return m_rotation; }
    bool flipH() const { return m_flipH; }
    bool flipV() const { return m_flipV; }

    Q_INVOKABLE void rotateLeft();
    Q_INVOKABLE void rotateRight();
    Q_INVOKABLE void flipHorizontal();
    Q_INVOKABLE void flipVertical();
    Q_INVOKABLE void resetTransform();
    Q_INVOKABLE void toggleFullscreen();

signals:
    void pathChanged();
    void infoChanged();
    void transformChanged();
    void displayChanged();     // path or transform changed -> source changed
    void toggleFullscreenRequested();

private:
    void readMetadata();

    QString m_path;
    QString m_format;
    QString m_fileName;
    qint64 m_fileSize = 0;
    int m_imgW = 0, m_imgH = 0;
    int m_rotation = 0;
    bool m_flipH = false, m_flipV = false;
};
