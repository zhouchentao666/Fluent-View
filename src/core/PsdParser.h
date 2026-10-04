#pragma once

#include "ImageTypes.h"

class QByteArray;
class QString;

namespace fv {

// Self-contained reader for Adobe Photoshop PSD/PSB files.
// Decodes the flattened composite image (the same preview Photoshop shows
// when "Maximize Compatibility" is on) and extracts layer names/bounds.
class PsdParser {
public:
    struct Psd {
        QImage              image;
        QList<PsdLayerInfo> layers;
        QString             error;
        QString             colorMode;   // "RGB", "Grayscale", "CMYK", ...
        int                 bitDepth = 8;
    };

    static Psd parse(const QString &filePath);
};

} // namespace fv
