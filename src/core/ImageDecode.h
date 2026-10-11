#pragma once

#include <QImage>
#include <QSize>
#include <QString>

class ImageCache;

namespace ImageDecode {

// Hard safety cap so a 100+ MP image can never exhaust memory on decode.
constexpr int MAX_DECODE_SIDE = 8192;

/**
 * Decode an image file off the GUI thread.
 *
 * @param path         Absolute file path.
 * @param requestedSize Desired output box (0,0 means "as large as possible,
 *                     capped by MAX_DECODE_SIDE). The result preserves aspect.
 * @param rotation     Rotation in degrees (0/90/180/270).
 * @param flipH        Mirror horizontally.
 * @param flipV        Mirror vertically.
 * @param cache        Optional LRU cache to consult / populate.
 * @param error        Receives a human readable error on failure.
 * @return decoded (and transformed) image, possibly null on error.
 */
QImage decode(const QString &path,
              const QSize &requestedSize,
              int rotation,
              bool flipH,
              bool flipV,
              ImageCache *cache,
              QString *error = nullptr);

} // namespace ImageDecode
