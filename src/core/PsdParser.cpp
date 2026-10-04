#include "PsdParser.h"

#include <QFile>
#include <QtEndian>

namespace fv {

namespace {

inline quint16 be16(const uchar *p) { return qFromBigEndian<quint16>(p); }
inline quint32 be32(const uchar *p) { return qFromBigEndian<quint32>(p); }
inline quint64 be64(const uchar *p) { return qFromBigEndian<quint64>(p); }

// Section length: PSB stores 64-bit lengths, PSD 32-bit. Returns bytes consumed
// by the length field itself.
inline int secLen(const uchar *p, quint16 version, quint64 &out)
{
    if (version == 2) { out = be64(p); return 8; }
    out = be32(p); return 4;
}

QByteArray unpackBits(const uchar *src, int srcLen, int expected)
{
    QByteArray out;
    out.reserve(expected);
    int i = 0;
    while (i < srcLen && out.size() < expected) {
        const signed char n = static_cast<signed char>(src[i++]);
        if (n >= 0) {                                    // literal run
            const int copy = qMin(int(n) + 1, srcLen - i);
            out.append(reinterpret_cast<const char *>(src + i), copy);
            i += copy;
        } else if (n != -128) {                          // repeat run
            const int reps = 1 - int(n);
            if (i >= srcLen) break;
            const char b = char(src[i++]);
            for (int k = 0; k < reps && out.size() < expected; ++k)
                out.append(b);
        }
        // -128 = no-op
    }
    while (out.size() < expected)
        out.append(char(0));
    return out;
}

struct Plane { QByteArray bytes; };

// Sample of plane `ch` at pixel index, normalized to 0..255.
inline int sample8(const QVector<Plane> &planes, int ch, qsizetype idx, int bps)
{
    if (ch >= planes.size()) return 0;
    const uchar *p = reinterpret_cast<const uchar *>(planes[ch].bytes.constData()) + idx * bps;
    return (bps == 2) ? int(be16(p) / 257) : int(p[0]);
}

QImage assemble(const QVector<Plane> &planes, quint32 w, quint32 h, quint16 depth,
                quint16 mode, const QByteArray &palette, QString &error)
{
    const int bps = depth / 8;

    if (mode == 0 || mode == 1) {            // Bitmap (1-bit)
        error = QStringLiteral("Bitmap-mode images are not supported");
        return QImage();
    }
    if (mode == 2 || mode == 8) {            // Grayscale / Duotone
        QImage img(int(w), int(h), QImage::Format_Grayscale8);
        for (quint32 y = 0; y < h; ++y) {
            uchar *line = img.scanLine(y);
            for (quint32 x = 0; x < w; ++x)
                line[x] = uchar(sample8(planes, 0, qsizetype(y) * w + x, bps));
        }
        return img;
    }
    if (mode == 3) {                          // Indexed
        if (palette.size() < 768) { error = QStringLiteral("Indexed PSD missing palette"); return QImage(); }
        QImage img(int(w), int(h), QImage::Format_RGB32);
        for (quint32 y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (quint32 x = 0; x < w; ++x) {
                const int idx = sample8(planes, 0, qsizetype(y) * w + x, bps) & 0xFF;
                line[x] = qRgb(uchar(palette[idx * 3]), uchar(palette[idx * 3 + 1]),
                               uchar(palette[idx * 3 + 2]));
            }
        }
        return img;
    }
    if (mode == 4) {                          // CMYK (inverted ink coverage)
        QImage img(int(w), int(h), QImage::Format_RGB32);
        const bool hasAlpha = planes.size() >= 5;
        for (quint32 y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (quint32 x = 0; x < w; ++x) {
                const qsizetype i = qsizetype(y) * w + x;
                const int c = 255 - sample8(planes, 0, i, bps);
                const int m = 255 - sample8(planes, 1, i, bps);
                const int yl = 255 - sample8(planes, 2, i, bps);
                const int k = 255 - sample8(planes, 3, i, bps);
                const int r = qBound(0, 255 * (255 - c) * (255 - k) / 65025, 255);
                const int g = qBound(0, 255 * (255 - m) * (255 - k) / 65025, 255);
                const int b = qBound(0, 255 * (255 - yl) * (255 - k) / 65025, 255);
                const int a = hasAlpha ? sample8(planes, 4, i, bps) : 255;
                line[x] = qRgba(r, g, b, a);
            }
        }
        return img;
    }

    // Modes 0 (RGB), 7 (Multichannel), 9 (Lab approximated): R G B [A]
    const bool hasAlpha = planes.size() >= 4;
    QImage img(int(w), int(h), hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    for (quint32 y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (quint32 x = 0; x < w; ++x) {
            const qsizetype i = qsizetype(y) * w + x;
            line[x] = qRgba(sample8(planes, 0, i, bps),
                            sample8(planes, 1, i, bps),
                            sample8(planes, 2, i, bps),
                            hasAlpha ? sample8(planes, 3, i, bps) : 255);
        }
    }
    return img;
}

bool decodeComposite(const uchar *d, int size, quint16 version,
                     quint16 channels, quint32 width, quint32 height,
                     quint16 depth, quint16 mode,
                     const QByteArray &palette, QImage &out, QString &error)
{
    if ((depth != 8 && depth != 16) || channels == 0 || channels > 6) {
        error = (depth == 1 || depth == 32)
                ? QStringLiteral("%1-bit images are not supported").arg(depth)
                : QStringLiteral("Unsupported channel/depth layout");
        return false;
    }
    const qsizetype pixels = qsizetype(width) * height;
    if (pixels > 400'000'000) {
        error = QStringLiteral("Image too large to decode (%1 MP)").arg(pixels / 1'000'000);
        return false;
    }
    if (size < 2) { error = QStringLiteral("No composite image data"); return false; }

    const int bps = depth / 8;
    const qsizetype planeBytes = pixels * bps;
    const quint16 compression = be16(d);
    int pos = 2;

    auto ok = [&](qsizetype off, qsizetype len) { return off >= 0 && off + len <= size; };

    QVector<Plane> planes;
    planes.reserve(channels);
    for (int ch = 0; ch < channels; ++ch) {
        Plane plane;
        plane.bytes.resize(planeBytes);
        if (compression == 0) {                              // raw
            if (!ok(pos, planeBytes)) { error = QStringLiteral("Truncated PSD data"); return false; }
            memcpy(plane.bytes.data(), d + pos, size_t(planeBytes));
            pos += int(planeBytes);
        } else if (compression == 1) {                       // PackBits RLE, row by row
            const int rowLenField = version == 2 ? 4 : 2;
            const qsizetype tableSize = qsizetype(height) * rowLenField;
            if (!ok(pos, tableSize)) { error = QStringLiteral("Truncated PSD data"); return false; }
            qsizetype rowsOff = pos;
            qsizetype dataOff = pos + tableSize;
            const int bpl = int(width) * bps;
            for (quint32 y = 0; y < height; ++y) {
                const qsizetype rowLen = version == 2
                        ? qsizetype(be32(d + rowsOff))
                        : qsizetype(be16(d + rowsOff));
                if (!ok(dataOff, rowLen)) { error = QStringLiteral("Truncated PSD data"); return false; }
                const QByteArray row = unpackBits(d + dataOff, int(rowLen), bpl);
                memcpy(plane.bytes.data() + qsizetype(y) * bpl, row.constData(),
                       size_t(qMin(row.size(), bpl)));
                rowsOff += rowLenField;
                dataOff += rowLen;
            }
            pos = int(dataOff);
        } else {
            error = QStringLiteral("ZIP-compressed composite is not supported; "
                                   "re-save the file with 'Maximize Compatibility' on");
            return false;
        }
        planes.append(plane);
    }

    out = assemble(planes, width, height, depth, mode, palette, error);
    return !out.isNull();
}

void parseLayerRecords(const uchar *d, int size, quint16 version,
                       qsizetype layerInfoData, qsizetype layerInfoEnd,
                       QList<PsdLayerInfo> &layers)
{
    if (layerInfoData + 2 > size)
        return;
    qint32 layerCount = qint16(be16(d + layerInfoData));
    if (layerCount < 0) layerCount = -layerCount;   // first alpha channel is transparency
    layerCount = qMin<int>(layerCount, 4096);

    qsizetype off = layerInfoData + 2;
    for (int i = 0; i < layerCount && off + 18 <= layerInfoEnd && off + 18 <= size; ++i) {
        PsdLayerInfo layer;
        const qint32 top    = qint32(be32(d + off));
        const qint32 left   = qint32(be32(d + off + 4));
        const qint32 bottom = qint32(be32(d + off + 8));
        const qint32 right  = qint32(be32(d + off + 12));
        layer.bounds = QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));

        const quint16 chanCount = be16(d + off + 16);
        off += 18 + qsizetype(chanCount) * (version == 2 ? 8 : 6);   // channel infos

        if (off + 16 > size) break;
        off += 4;                                                    // blend signature '8BIM'
        off += 4;                                                    // blend key
        layer.opacity = d[off] / 255.0;                              // u8 opacity
        off += 2;                                                    // opacity + clipping
        layer.visible = (d[off] & 0x02) == 0;                        // flags: bit1 = hidden
        off += 2;                                                    // flags + filler

        quint64 extraLen = 0;
        const int elb = secLen(d + off, version, extraLen);
        const qsizetype extraData = off + elb;
        const qsizetype extraEnd = extraData + qsizetype(extraLen);
        off = extraEnd;
        if (extraEnd > size) break;

        // Walk extra data: mask info, blending ranges, pascal name, then
        // additional info blocks (look for 'luni' = Unicode layer name).
        QString pascalName;
        qsizetype e = extraData;
        quint64 l2 = 0;
        int lb2 = 0;
        if (e + 4 <= extraEnd) {
            lb2 = secLen(d + e, version, l2);
            e += lb2 + qsizetype(l2);                                // layer mask info
        }
        if (e + 4 <= extraEnd) {
            lb2 = secLen(d + e, version, l2);
            e += lb2 + qsizetype(l2);                                // blending ranges
        }
        // Pascal name, padded to a multiple of 4.
        if (e < extraEnd) {
            const int nameLen = d[e];
            const int padded = (1 + nameLen + 3) & ~3;
            if (e + padded <= extraEnd) {
                pascalName = QString::fromLatin1(reinterpret_cast<const char *>(d + e + 1), nameLen);
                e += padded;
            }
        }
        // Additional info blocks: [sig 4][key 4][len 4|8][data padded to 4]
        const int lenW = version == 2 ? 8 : 4;
        while (e + 8 + lenW <= extraEnd) {
            const char *key = reinterpret_cast<const char *>(d + e + 4);
            quint64 blockLen = 0;
            const int blb = secLen(d + e + 8, version, blockLen);
            const qsizetype blockData = e + 8 + blb;
            if (blockData + qsizetype(blockLen) > extraEnd) break;
            if (memcmp(key, "luni", 4) == 0 && blockLen >= 4) {
                const quint32 chars = be32(d + blockData);
                if (chars <= 4096 && blockData + 4 + qsizetype(chars) * 2 <= extraEnd) {
                    QString name;
                    name.reserve(int(chars));
                    const uchar *nd = d + blockData + 4;
                    for (quint32 c = 0; c < chars; ++c)
                        name += QChar(qFromBigEndian<quint16>(nd + c * 2));
                    layer.name = name;
                }
            }
            e = blockData + qsizetype(blockLen);
        }
        if (layer.name.isEmpty())
            layer.name = pascalName.isEmpty() ? QStringLiteral("Layer %1").arg(i + 1) : pascalName;
        layers.append(layer);
    }
}

} // namespace

PsdParser::Psd PsdParser::parse(const QString &filePath)
{
    Psd result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Cannot open file");
        return result;
    }
    if (file.size() > qint64(2) * 1024 * 1024 * 1024) {
        result.error = QStringLiteral("Files larger than 2 GB are not supported");
        return result;
    }
    const QByteArray data = file.readAll();
    file.close();

    const uchar *d = reinterpret_cast<const uchar *>(data.constData());
    const int size = data.size();
    if (size < 26 || memcmp(d, "8BPS", 4) != 0) {
        result.error = QStringLiteral("Not a PSD/PSB file");
        return result;
    }

    const quint16 version = be16(d + 4);
    if (version != 1 && version != 2) {
        result.error = QStringLiteral("Unsupported PSD version %1").arg(version);
        return result;
    }
    const quint16 channels = be16(d + 12);
    const quint32 height   = be32(d + 14);
    const quint32 width    = be32(d + 18);
    const quint16 depth    = be16(d + 22);
    const quint16 mode     = be16(d + 24);

    result.bitDepth = depth;
    switch (mode) {
    case 0:  result.colorMode = QStringLiteral("Bitmap"); break;
    case 1:  result.colorMode = QStringLiteral("Grayscale"); break;
    case 2:  result.colorMode = QStringLiteral("Indexed"); break;
    case 3:  result.colorMode = QStringLiteral("RGB"); break;
    case 4:  result.colorMode = QStringLiteral("CMYK"); break;
    case 7:  result.colorMode = QStringLiteral("Multichannel"); break;
    case 8:  result.colorMode = QStringLiteral("Duotone"); break;
    case 9:  result.colorMode = QStringLiteral("Lab"); break;
    default: result.colorMode = QStringLiteral("Mode %1").arg(mode); break;
    }

    int p = 26;
    quint64 len = 0;
    int lenBytes = 0;

    // --- color mode data (palette for indexed mode) ---
    if (p + 4 > size) { result.error = QStringLiteral("Corrupted PSD"); return result; }
    lenBytes = secLen(d + p, version, len);
    p += lenBytes;
    QByteArray palette;
    if (len) {
        if (p + int(len) > size) { result.error = QStringLiteral("Corrupted PSD"); return result; }
        palette = QByteArray(reinterpret_cast<const char *>(d + p), int(qMin<quint64>(len, 768)));
    }
    p += int(len);

    // --- image resources ---
    if (p + 4 > size) { result.error = QStringLiteral("Corrupted PSD"); return result; }
    lenBytes = secLen(d + p, version, len);
    p += lenBytes + int(len);
    if (p > size) { result.error = QStringLiteral("Corrupted PSD"); return result; }

    // --- layer & mask information: ['8BIM'][length][data] ---
    qsizetype mergedPos = p;
    if (p + 8 <= size && memcmp(d + p, "8BIM", 4) == 0) {
        lenBytes = secLen(d + p + 4, version, len);
        const qsizetype sectionData = p + 4 + lenBytes;
        const qsizetype sectionEnd = sectionData + qsizetype(len);
        if (len > 0 && sectionEnd <= size) {
            // Layer info sub-section: [length][layer count][records...]
            if (sectionData + 8 <= size) {
                quint64 layerInfoLen = 0;
                const int lib = secLen(d + sectionData, version, layerInfoLen);
                const qsizetype layerInfoData = sectionData + lib;
                const qsizetype layerInfoEnd = layerInfoData + qsizetype(layerInfoLen);
                if (layerInfoLen > 0 && layerInfoEnd <= sectionEnd)
                    parseLayerRecords(d, size, version, layerInfoData, layerInfoEnd, result.layers);
            }
        }
        mergedPos = qMax(sectionEnd, qsizetype(p + 4 + lenBytes));
    }

    // --- merged composite image data ---
    if (mergedPos < 0 || mergedPos + 2 > size) {
        result.error = QStringLiteral("No composite image data; re-save with "
                                      "'Maximize Compatibility' enabled");
        return result;
    }
    QString err;
    QImage img;
    if (!decodeComposite(d + mergedPos, size - int(mergedPos), version, channels,
                         width, height, depth, mode, palette, img, err)) {
        result.error = err;
        return result;
    }
    result.image = img;
    return result;
}

} // namespace fv
