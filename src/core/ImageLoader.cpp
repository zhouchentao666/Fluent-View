#include "ImageLoader.h"

#include "ExifParser.h"
#include "FormatRegistry.h"
#include "PsdParser.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMovie>
#include <QPainter>
#include <QSvgRenderer>
#include <QThreadPool>
#include <QUrl>

#ifdef FV_HAVE_LIBRAW
#  include <libraw/libraw.h>
#endif
#ifdef FV_HAVE_LIBHEIF
#  include <libheif/heif.h>
#endif

namespace fv {

ImageLoader &ImageLoader::instance()
{
    static ImageLoader loader;
    return loader;
}

ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent)
{
}

bool ImageLoader::isAnimatedFile(const QString &path) const
{
    static const QSet<QString> animated = {
        QStringLiteral("gif"), QStringLiteral("webp"), QStringLiteral("apng"),
        QStringLiteral("png"), QStringLiteral("mng"),  QStringLiteral("svg"),
        QStringLiteral("svgz")
    };
    const QString suffix = QFileInfo(path).suffix().toLower();
    // Only trust QImageReader for animation capability on GIF-family formats;
    // PNG is included because APNG shares the suffix.
    if (suffix == QLatin1String("gif") || suffix == QLatin1String("mng") ||
        suffix == QLatin1String("svg") || suffix == QLatin1String("svgz"))
        return true;
    if (suffix == QLatin1String("webp") || suffix == QLatin1String("apng") ||
        suffix == QLatin1String("png")) {
        QImageReader r(path);
        return r.supportsAnimation() && r.imageCount() > 1;
    }
    return false;
}

void ImageLoader::load(const QString &path)
{
    if (m_inFlight.contains(path))
        return;
    m_inFlight.insert(path);

    const QString canonical = path;
    QThreadPool::globalInstance()->start([this, canonical]() {
        LoadResult result = decode(canonical);
        // Deliver on the loader's thread (GUI); stale results are filtered by callers.
        QMetaObject::invokeMethod(this, [this, canonical, result]() {
            m_inFlight.remove(canonical);
            if (result.ok())
                emit loaded(canonical, result);
            else
                emit loadFailed(canonical, result.error, QStringList(), false);
        }, Qt::QueuedConnection);
    });
}

void ImageLoader::cancelPending()
{
    m_inFlight.clear();
}

// --------------------------------------------------------------------------
// Decoders
// --------------------------------------------------------------------------
LoadResult ImageLoader::decode(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists()) {
        LoadResult missing;
        missing.path = path;
        missing.error = QStringLiteral("File not found");
        return missing;
    }

    const QString suffix = fi.suffix().toLower();
    const Decoder dec = FormatRegistry::instance().decoderFor(suffix);

    LoadResult result;
    switch (dec) {
    case Decoder::Svg:         result = decodeSvg(path, suffix); break;
    case Decoder::Psd:         result = decodePsd(path, suffix); break;
    case Decoder::LibRaw:      result = decodeRaw(path, suffix); break;
    case Decoder::LibHeif:     result = decodeHeif(path, suffix); break;
    case Decoder::Unsupported: result = decodeEps(path, suffix); break;
    case Decoder::QtPlugins:
    default:                   result = decodeQtPlugins(path, suffix); break;
    }

    if (result.ok()) {
        result.path = path;
        // EXIF only for formats that embed it and that we read fully into memory.
        if (result.exif.valid == false &&
            (suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg") ||
             suffix == QLatin1String("jpe") || suffix == QLatin1String("jfif") ||
             suffix == QLatin1String("tif") || suffix == QLatin1String("tiff"))) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                const QByteArray head = f.read(512 * 1024); // EXIF lives up front
                result.exif = ExifParser::read(head, suffix);
            }
        }
    }
    return result;
}

LoadResult ImageLoader::decodeQtPlugins(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();

    QImageReader reader(path);
    reader.setAutoTransform(true);   // honor EXIF orientation
    const QImage image = reader.read();
    if (image.isNull()) {
        result.error = reader.errorString();
        return result;
    }
    result.image = image;
    result.decoder = QStringLiteral("Qt %1").arg(reader.format().toUpper());
    if (suffix == QLatin1String("xcf") || suffix == QLatin1String("kra") ||
        suffix == QLatin1String("ora") || suffix == QLatin1String("jxl") ||
        suffix == QLatin1String("qoi") || suffix == QLatin1String("ai") ||
        suffix == QLatin1String("eps") || suffix == QLatin1String("pdf")) {
        result.warnings.append(QStringLiteral("Format requires an optional Qt imageformat plugin; "
                                               "if this fails, the file cannot be displayed"));
    }
    return result;
}

LoadResult ImageLoader::decodeSvg(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Cannot open file");
        return result;
    }
    const QByteArray data = f.readAll();
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        result.error = QStringLiteral("Invalid SVG document");
        return result;
    }
    QSizeF view = renderer.viewBoxF().size();
    if (view.isEmpty()) view = QSizeF(1024, 1024);
    // Render at high resolution for crisp zooming (cap for memory safety).
    const double scale = qBound(1.0, 4096.0 / qMax(view.width(), view.height()), 4.0);
    QImage image(int(view.width() * scale), int(view.height() * scale),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    renderer.render(&p, QRectF(QPointF(0, 0), QSizeF(image.size())));
    p.end();

    result.image = image;
    result.decoder = QStringLiteral("SVG (vector)");
    result.exif.pixelX = image.width();
    result.exif.pixelY = image.height();
    Q_UNUSED(suffix);
    return result;
}

LoadResult ImageLoader::decodePsd(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();

    PsdParser::Psd psd = PsdParser::parse(path);
    if (!psd.image.isNull()) {
        result.image = std::move(psd.image);
        result.layers = std::move(psd.layers);
        result.decoder = QStringLiteral("Built-in PSD%1").arg(
            suffix == QLatin1String("psb") ? QStringLiteral("B") : QString());
        result.exif.pixelX = result.image.width();
        result.exif.pixelY = result.image.height();
    } else {
        result.error = psd.error;
    }
    return result;
}

LoadResult ImageLoader::decodeRaw(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();

#ifdef FV_HAVE_LIBRAW
    LibRaw raw;
    const int ret = raw.open_file(QFile::encodeName(path).constData());
    if (ret != LIBRAW_SUCCESS) {
        result.error = QStringLiteral("LibRaw error %1").arg(ret);
        return result;
    }
    if (raw.unpack() != LIBRAW_SUCCESS) {
        result.error = QStringLiteral("Cannot decode RAW data");
        return result;
    }
    raw.imgdata.params.use_camera_wb = 1;
    raw.imgdata.params.output_bps = 8;
    raw.imgdata.params.no_auto_bright = 0;
    if (raw.dcraw_process() != LIBRAW_SUCCESS) {
        result.error = QStringLiteral("Cannot process RAW data");
        return result;
    }
    const int w = raw.imgdata.sizes.iwidth;
    const int h = raw.imgdata.sizes.iheight;
    QImage image(w, h, QImage::Format_RGB32);
    const ushort *pixels = raw.imgdata.image;
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const ushort *px = pixels + (qsizetype(y) * w + x) * 4;
            line[x] = qRgb(qMin(255, px[0] >> 8), qMin(255, px[1] >> 8), qMin(255, px[2] >> 8));
        }
    }
    result.image = image;
    result.decoder = QStringLiteral("LibRaw");
    result.exif.cameraMake = QString::fromLatin1(raw.imgdata.idata.make).trimmed();
    result.exif.cameraModel = QString::fromLatin1(raw.imgdata.idata.model).trimmed();
    result.exif.valid = true;
    result.exif.pixelX = w;
    result.exif.pixelY = h;
#else
    Q_UNUSED(suffix);
    result.error = QStringLiteral("RAW support is not built into this binary");
#endif
    return result;
}

LoadResult ImageLoader::decodeHeif(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();

#ifdef FV_HAVE_LIBHEIF
    heif_context *ctx = heif_context_alloc();
    heif_error err;
    heif_context_read_from_file(ctx, QFile::encodeName(path).constData(), &err);
    if (err.code != heif_error_Ok) {
        result.error = QString::fromUtf8(err.message);
        heif_context_free(ctx);
        return result;
    }
    heif_image_handle *handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok || !handle) {
        result.error = QString::fromUtf8(err.message);
        heif_context_free(ctx);
        return result;
    }
    heif_image *img = nullptr;
    err = heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGB, &err);
    if (err.code == heif_error_Ok && img) {
        int stride = 0;
        const uint8_t *data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);
        const int w = heif_image_get_width(img, heif_channel_interleaved);
        const int h = heif_image_get_height(img, heif_channel_interleaved);
        QImage image(w, h, QImage::Format_RGB32);
        if (data && stride > 0) {
            for (int y = 0; y < h; ++y) {
                memcpy(image.scanLine(y), data + qsizetype(y) * stride, size_t(w) * 3);
            }
        }
        result.image = image;
        result.decoder = QStringLiteral("libheif");
        result.exif.pixelX = w;
        result.exif.pixelY = h;
        heif_image_release(img);
    } else {
        result.error = QString::fromUtf8(err.message);
    }
    heif_image_handle_release(handle);
    heif_context_free(ctx);
#else
    Q_UNUSED(suffix);
    result.error = QStringLiteral("HEIC/AVIF support is not built into this binary");
#endif
    return result;
}

LoadResult ImageLoader::decodeEps(const QString &path, const QString &suffix)
{
    LoadResult result;
    QFileInfo fi(path);
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();
    Q_UNUSED(suffix);

    // EPS files carry a TIFF/WMF preview; try to pull the TIFF block out.
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Cannot open file");
        return result;
    }
    const QByteArray data = f.read(qint64(8 * 1024 * 1024));
    static const char tiffLe[] = { 'I', 'I', 0x2A, 0x00 };
    static const char tiffBe[] = { 'M', 'M', 0x00, 0x2A };
    const int idx = data.indexOf(QByteArray(tiffLe, 4));
    const int idxBe = data.indexOf(QByteArray(tiffBe, 4));
    int start = -1;
    if (idx >= 0 && (idxBe < 0 || idx < idxBe)) start = idx;
    else if (idxBe >= 0) start = idxBe;
    if (start < 0) {
        result.error = QStringLiteral("No embedded TIFF preview found in EPS");
        return result;
    }
    QByteArray buffer = data.mid(start);
    QBuffer buf(&buffer);
    buf.open(QIODevice::ReadOnly);
    QImageReader r(&buf, QByteArrayLiteral("tiff"));
    QImage image = r.read();
    if (!image.isNull()) {
        result.image = image;
        result.decoder = QStringLiteral("EPS preview (TIFF)");
    } else {
        result.error = QStringLiteral("Cannot decode embedded EPS preview");
    }
    return result;
}

// --------------------------------------------------------------------------
// Errors / info rows
// --------------------------------------------------------------------------
ImageLoader::ErrorKind ImageLoader::classifyError(const QString &readerError)
{
    if (readerError.contains(QLatin1String("Unable to read"), Qt::CaseInsensitive) ||
        readerError.contains(QLatin1String("truncated"), Qt::CaseInsensitive) ||
        readerError.contains(QLatin1String("corrupt"), Qt::CaseInsensitive))
        return ErrorKind::Truncated;
    if (readerError.contains(QLatin1String("unsupported"), Qt::CaseInsensitive) ||
        readerError.contains(QLatin1String("Unknown format"), Qt::CaseInsensitive))
        return ErrorKind::UnsupportedFormat;
    if (readerError.contains(QLatin1String("memory"), Qt::CaseInsensitive) ||
        readerError.contains(QLatin1String("allocation"), Qt::CaseInsensitive))
        return ErrorKind::OutOfMemory;
    return ErrorKind::Unknown;
}

QStringList ImageLoader::buildFileInfoRows(const QFileInfo &fi, const QString &decoderName,
                                           const QString &extra)
{
    const QString unit = fi.size() > 1024 * 1024
            ? QStringLiteral("%1 MB").arg(double(fi.size()) / 1024 / 1024, 0, 'f', 1)
            : QStringLiteral("%1 KB").arg(double(fi.size()) / 1024, 0, 'f', 1);
    QStringList rows;
    rows << fi.fileName()
         << fi.absolutePath()
         << unit
         << (decoderName + (extra.isEmpty() ? QString() : " - " + extra))
         << fi.lastModified().toString(Qt::SystemLocaleLongDate);
    return rows;
}

QImage ImageLoader::preview(const QString &path, const QSize &maxSize, QString *error)
{
    const QFileInfo fi(path);
    if (!fi.exists()) {
        if (error) *error = QStringLiteral("File not found");
        return QImage();
    }
    const QString suffix = fi.suffix().toLower();
    const Decoder dec = FormatRegistry::instance().decoderFor(suffix);

    if (dec == Decoder::Svg) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return QImage();
        QSvgRenderer renderer(f.readAll());
        if (!renderer.isValid()) return QImage();
        QSizeF view = renderer.viewBoxF().size();
        if (view.isEmpty()) view = QSizeF(256, 256);
        const double scale = qMin(maxSize.width() / view.width(),
                                  maxSize.height() / view.height());
        QImage img(qMax(1, int(view.width() * scale)), qMax(1, int(view.height() * scale)),
                   QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        renderer.render(&p, QRectF(QPointF(0, 0), QSizeF(img.size())));
        p.end();
        return img;
    }
    if (dec == Decoder::Psd) {
        PsdParser::Psd psd = PsdParser::parse(path);
        if (psd.image.isNull()) {
            if (error) *error = psd.error;
            return QImage();
        }
        return psd.image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (dec == Decoder::LibRaw) {
#ifdef FV_HAVE_LIBRAW
        // Use the embedded JPEG thumb when present - much faster than full decode.
        LibRaw raw;
        if (raw.open_file(QFile::encodeName(path).constData()) == LIBRAW_SUCCESS) {
            libraw_processed_image_t *thumb = raw.dcraw_make_mem_thumb();
            if (thumb) {
                QImage img;
                if (img.loadFromData(thumb->data, int(thumb->data_size)))
                    return img.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                LibRaw::dcraw_clear_mem(thumb);
            }
        }
#endif
        LoadResult r = decodeRaw(path, suffix);
        if (error && !r.ok()) *error = r.error;
        return r.image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (dec == Decoder::LibHeif) {
#ifdef FV_HAVE_LIBHEIF
        // Full decode then scale down.
        LoadResult r = decodeHeif(path, suffix);
        if (r.ok())
            return r.image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (error) *error = r.error;
        return QImage();
#else
        if (error) *error = QStringLiteral("HEIC/AVIF support is not built in");
        return QImage();
#endif
    }

    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize sz = reader.size();
    if (sz.isValid() && (sz.width() > maxSize.width() * 2 || sz.height() > maxSize.height() * 2))
        reader.setScaledSize(sz.scaled(maxSize, Qt::KeepAspectRatio));
    QImage img = reader.read();
    if (img.isNull() && error)
        *error = reader.errorString();
    if (!img.isNull())
        img = img.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}

} // namespace fv
