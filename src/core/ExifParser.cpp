#include "ExifParser.h"

#include <QtEndian>
#include <QDateTime>

namespace fv {

namespace {

struct BeLe {
    const uchar *d = nullptr;
    int n = 0;
    bool little = true;

    bool ok(int off, int len) const { return off >= 0 && len >= 0 && off + len <= n; }
    quint16 u16(int off) const
    {
        if (!ok(off, 2)) return 0;
        return little ? qFromLittleEndian<quint16>(d + off)
                      : qFromBigEndian<quint16>(d + off);
    }
    quint32 u32(int off) const
    {
        if (!ok(off, 4)) return 0;
        return little ? qFromLittleEndian<quint32>(d + off)
                      : qFromBigEndian<quint32>(d + off);
    }
    double rational(int off) const
    {
        if (!ok(off, 8)) return 0;
        const quint32 num = u32(off);
        const quint32 den = u32(off + 4);
        return den ? double(num) / double(den) : 0.0;
    }
    QString ascii(int off, int count) const
    {
        if (!ok(off, count)) return QString();
        const QByteArray raw(reinterpret_cast<const char *>(d + off), count);
        QString s = QString::fromLatin1(raw);
        s.remove(QLatin1Char('\0'));
        return s.trimmed();
    }
    QByteArray bytes(int off, int count) const
    {
        if (!ok(off, count)) return QByteArray();
        return QByteArray(reinterpret_cast<const char *>(d + off), count);
    }
};

int typeSize(int type)
{
    static const int sizes[] = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
    return (type >= 1 && type <= 12) ? sizes[type] : 0;
}

double gpsCoordinate(const BeLe &r, int valueOffset, int count)
{
    // GPSLatitude/GPSLongitude: 3 RATIONALs (deg, min, sec)
    double v = 0;
    if (count >= 3) {
        const double deg = r.rational(valueOffset);
        const double min = r.rational(valueOffset + 8);
        const double sec = r.rational(valueOffset + 16);
        v = deg + min / 60.0 + sec / 3600.0;
    } else if (count == 1) {
        v = r.rational(valueOffset);
    }
    return v;
}

} // namespace

ExifData ExifParser::read(const QByteArray &data, const QString &suffix)
{
    const QString s = suffix.toLower();

    // TIFF-family files are EXIF containers themselves.
    if (s == QLatin1String("tif") || s == QLatin1String("tiff") ||
        s == QLatin1String("nef") || s == QLatin1String("arw") ||
        s == QLatin1String("cr2") || s == QLatin1String("dng") ||
        s == QLatin1String("orf") || s == QLatin1String("rw2") ||
        s == QLatin1String("raf") || s == QLatin1String("pef")) {
        if (data.size() > 8)
            return readTiff(reinterpret_cast<const uchar *>(data.constData()), data.size());
        return {};
    }

    // JPEG: walk markers looking for APP1 / "Exif\0\0".
    if (data.size() > 4 && uchar(data[0]) == 0xFF && uchar(data[1]) == 0xD8) {
        int pos = 2;
        while (pos + 4 < data.size()) {
            if (uchar(data[pos]) != 0xFF) break;
            uchar marker = uchar(data[pos + 1]);
            if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7)) { pos += 2; continue; }
            if (marker == 0xDA || marker == 0xD9) break; // start of image data
            const int len = (uchar(data[pos + 2]) << 8) | uchar(data[pos + 3]);
            if (len < 2) break;
            if (marker == 0xE1) {
                const int seg = pos + 4;
                const int segLen = len - 2;
                static const char exifId[] = { 'E', 'x', 'i', 'f', '\0', '\0' };
                if (segLen > 6 && memcmp(data.constData() + seg, exifId, 6) == 0)
                    return readTiff(reinterpret_cast<const uchar *>(data.constData()) + seg + 6,
                                    segLen - 6);
            }
            pos += 2 + len;
        }
    }
    return {};
}

ExifData ExifParser::readTiff(const uchar *data, int size)
{
    BeLe r{ data, size, true };
    if (size < 8) return {};
    if (data[0] == 'I' && data[1] == 'I') r.little = true;
    else if (data[0] == 'M' && data[1] == 'M') r.little = false;
    else return {};

    if (r.u16(2) != 42) return {};
    const int ifd0 = int(r.u32(4));
    if (ifd0 <= 0 || ifd0 >= size) return {};

    ExifData out;
    readIfd(data, size, ifd0, out, false);
    out.valid = true;
    return out;
}

void ExifParser::readIfd(const uchar *data, int size, int ifdOffset,
                         ExifData &out, bool gpsIfd)
{
    BeLe r{ data, size, data[0] == 'I' };
    const int count = r.u16(ifdOffset);
    if (count <= 0 || count > 4096) return;

    int exifIfd = 0;
    int gpsIfdOff = 0;

    for (int i = 0; i < count; ++i) {
        const int e = ifdOffset + 2 + i * 12;
        if (!r.ok(e, 12)) return;
        const quint16 tag   = r.u16(e);
        const quint16 type  = r.u16(e + 2);
        const quint32 cnt   = r.u32(e + 4);
        const int ts = typeSize(type);
        if (ts == 0) continue;

        const int total = int(cnt) * ts;
        const int valueOff = total <= 4 ? e + 8 : int(r.u32(e + 8));
        if (total > 4 && !r.ok(valueOff, total)) continue;

        auto str = [&]{ return r.ascii(valueOff, int(qMin<quint32>(cnt, 256))); };
        auto firstRational = [&]{ return r.rational(valueOff); };

        if (gpsIfd) {
            switch (tag) {
            case 0x0001: out.gpsLatRef = r.ascii(valueOff, 1).toUpper(); break;
            case 0x0002: out.gpsLatitude = gpsCoordinate(r, valueOff, int(cnt)); break;
            case 0x0003: out.gpsLonRef = r.ascii(valueOff, 1).toUpper(); break;
            case 0x0004: out.gpsLongitude = gpsCoordinate(r, valueOff, int(cnt)); break;
            case 0x0005: // GPSAltitudeRef: 0 above sea, 1 below
                if (cnt == 1 && r.u16(valueOff) == 1) out.gpsAltitude = -qAbs(out.gpsAltitude);
                break;
            case 0x0006: out.gpsAltitude = firstRational(); break;
            }
            continue;
        }

        switch (tag) {
        case 0x010F: out.cameraMake = str(); break;
        case 0x0110: out.cameraModel = str(); break;
        case 0x0112: out.orientation = int(r.u16(valueOff)); break;
        case 0x0131: out.software = str(); break;
        case 0x0132: if (out.dateTime.isEmpty()) out.dateTime = str(); break;
        case 0x013B: out.artist = str(); break;
        case 0x8769: exifIfd = int(r.u32(e + 8)); break;
        case 0x8825: gpsIfdOff = int(r.u32(e + 8)); break;
        case 0x0100: out.pixelX = int(r.u16(valueOff)); break;   // ImageWidth (SHORT)
        case 0x0101: out.pixelY = int(r.u16(valueOff)); break;   // ImageLength
        case 0x829A: out.exposureTime = firstRational(); break;
        case 0x829D: out.fNumber = firstRational(); break;
        case 0x8827: // ISO: SHORT/LONG, possibly multiple values
            out.iso = (type == 3) ? int(r.u16(valueOff)) : int(r.u32(valueOff));
            break;
        case 0x9003: out.dateTime = str(); break;
        case 0x920A: out.focalLength = firstRational(); break;
        case 0xA002: out.pixelX = int(r.u32(valueOff)); break;   // ExifImageWidth
        case 0xA003: out.pixelY = int(r.u32(valueOff)); break;
        case 0xA434: out.lensModel = str(); break;
        default: break;
        }
    }

    if (exifIfd > 0 && exifIfd < size)
        readIfd(data, size, exifIfd, out, false);
    if (gpsIfdOff > 0 && gpsIfdOff < size)
        readIfd(data, size, gpsIfdOff, out, true);

    // Apply hemisphere signs now that refs are known.
    if (out.gpsLatRef == QLatin1String("S")) out.gpsLatitude = -qAbs(out.gpsLatitude);
    if (out.gpsLonRef == QLatin1String("W")) out.gpsLongitude = -qAbs(out.gpsLongitude);
}

} // namespace fv
