#pragma once

#include <QImage>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace fv {

// EXIF metadata extracted by our own lightweight parser (JPEG/TIFF).
struct ExifData {
    bool valid = false;
    QString cameraMake;
    QString cameraModel;
    QString lensModel;
    QString software;
    QString dateTime;          // EXIF DateTimeOriginal if present
    QString artist;
    double  exposureTime = 0;  // seconds
    double  fNumber = 0;
    int     iso = 0;
    double  focalLength = 0;   // mm
    int     orientation = 1;   // 1..8
    double  gpsLatitude = qQNaN();
    double  gpsLongitude = qQNaN();
    double  gpsAltitude = qQNaN();
    QString gpsLatRef;         // "N" / "S" (internal, sign applied after parse)
    QString gpsLonRef;         // "E" / "W"
    int     pixelX = 0;
    int     pixelY = 0;
};

// Layer summary parsed from PSD/PSB files (names + bounds only).
struct PsdLayerInfo {
    QString name;
    QRect   bounds;
    double  opacity = 1.0;
    bool    visible = true;
};

// Result of decoding one file.
struct LoadResult {
    QString       path;
    QString       decoder;    // which backend produced the image
    QImage        image;
    ExifData      exif;
    QList<PsdLayerInfo> layers;   // non-empty for PSD/PSB
    QStringList   warnings;
    QString       error;      // empty on success
    qint64        fileSize = 0;
    QDateTime     lastModified;
    bool          ok() const { return !image.isNull(); }
};

inline QString formatExposure(double s)
{
    if (s <= 0) return QString();
    if (s >= 1.0) return QString::number(s, 'f', s >= 10 ? 0 : 1) + QLatin1Char('s');
    return QStringLiteral("1/%1s").arg(qRound(1.0 / s));
}

} // namespace fv

Q_DECLARE_METATYPE(fv::LoadResult)
