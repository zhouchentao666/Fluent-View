#pragma once

#include "ImageTypes.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QSet>

namespace fv {

// Central decode pipeline. Decoding runs on worker threads; results are
// delivered through the loaded() signal. Animated formats (GIF/WebP/APNG/MNG)
// are reported so the view can play them with QMovie.
class ImageLoader : public QObject {
    Q_OBJECT
public:
    static ImageLoader &instance();

    void load(const QString &path);   // async full decode; results via signals
    void cancelPending();             // drop results for stale requests
    bool isBusy() const { return !m_inFlight.isEmpty(); }

    bool isAnimatedFile(const QString &path) const;

    // Small preview for thumbnails; safe to call from any thread.
    static QImage preview(const QString &path, const QSize &maxSize, QString *error = nullptr);

signals:
    void loaded(const QString &path, const fv::LoadResult &result);
    void loadFailed(const QString &path, const QString &error,
                    const QStringList &fileInfoRows, bool unknownFormat);

private:
    explicit ImageLoader(QObject *parent = nullptr);

    enum class ErrorKind { None, Truncated, UnsupportedFormat, OutOfMemory, Unknown };

    static LoadResult decode(const QString &path);
    static LoadResult decodeQtPlugins(const QString &path, const QString &suffix);
    static LoadResult decodeSvg(const QString &path, const QString &suffix);
    static LoadResult decodePsd(const QString &path, const QString &suffix);
    static LoadResult decodeRaw(const QString &path, const QString &suffix);
    static LoadResult decodeHeif(const QString &path, const QString &suffix);
    static LoadResult decodeEps(const QString &path, const QString &suffix);

    static ErrorKind classifyError(const QString &readerError);
    static QStringList buildFileInfoRows(const QFileInfo &fi, const QString &decoderName,
                                         const QString &extra);

    QSet<QString> m_inFlight;
};

} // namespace fv
