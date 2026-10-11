#include "ImageDecode.h"
#include "ImageCache.h"

#include <QImageReader>
#include <QFileInfo>
#include <QTransform>

static QString makeKey(const QString &path, const QSize &req,
                       int rotation, bool flipH, bool flipV) {
    return QString("%1|%2x%3|r%4|h%5|v%6")
        .arg(path)
        .arg(req.width()).arg(req.height())
        .arg(rotation).arg(flipH ? 1 : 0).arg(flipV ? 1 : 0);
}

// Fit @p src inside @p box preserving aspect ratio; returns a valid non-empty size.
static QSize fitInside(const QSize &src, const QSize &box) {
    if (src.isEmpty() || box.isEmpty())
        return src;
    const qreal r = qMin(static_cast<qreal>(box.width()) / src.width(),
                         static_cast<qreal>(box.height()) / src.height());
    if (r >= 1.0)
        return src;
    return QSize(qMax(1, int(src.width() * r)), qMax(1, int(src.height() * r)));
}

namespace ImageDecode {

QImage decode(const QString &path,
              const QSize &requestedSize,
              int rotation,
              bool flipH,
              bool flipV,
              ImageCache *cache,
              QString *error) {
    if (!QFileInfo::exists(path)) {
        if (error) *error = "File does not exist";
        return {};
    }

    const QSize capped = (requestedSize.isEmpty()
        ? QSize(MAX_DECODE_SIDE, MAX_DECODE_SIDE)
        : fitInside(requestedSize, QSize(MAX_DECODE_SIDE, MAX_DECODE_SIDE)));

    const QString key = makeKey(path, capped, rotation, flipH, flipV);
    if (cache) {
        QImage cached;
        if (cache->get(key, cached))
            return cached;
    }

    QImageReader reader(path);
    reader.setAutoTransform(true); // honor EXIF orientation

    QSize target = capped;
    const QSize native = reader.size();
    if (!native.isEmpty())
        target = fitInside(native, capped);
    if (!target.isEmpty() && target != native)
        reader.setScaledSize(target);

    QImage img = reader.read();
    if (img.isNull()) {
        if (error) *error = reader.errorString();
        return {};
    }

    // Apply rotation / flips in image space (cheap, done off GUI thread).
    if (rotation != 0 || flipH || flipV) {
        QTransform t;
        if (flipH) t.scale(-1, 1);
        if (flipV) t.scale(1, -1);
        if (rotation != 0) t.rotate(rotation);
        // Compose so that flip happens before rotation visually.
        QTransform composed;
        if (rotation != 0) composed.rotate(rotation);
        if (flipH) composed.scale(-1, 1);
        if (flipV) composed.scale(1, -1);
        img = img.transformed(composed, Qt::SmoothTransformation);
    }

    if (cache)
        cache->insert(key, img);

    return img;
}

} // namespace ImageDecode
