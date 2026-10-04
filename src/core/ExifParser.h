#pragma once

#include "ImageTypes.h"

class QByteArray;

namespace fv {

// Minimal, dependency-free EXIF reader covering the tags the UI displays.
class ExifParser {
public:
    // Extracts EXIF from a JPEG or TIFF byte stream; returns exif.valid == false
    // when nothing useful is found (never fails hard).
    static ExifData read(const QByteArray &data, const QString &suffix);

private:
    static ExifData readTiff(const uchar *data, int size);
    static void readIfd(const uchar *data, int size, int ifdOffset,
                        ExifData &out, bool gpsIfd);
};

} // namespace fv
