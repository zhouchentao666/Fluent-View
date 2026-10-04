#pragma once

#include <QString>
#include <QStringList>

namespace fv {

// How a format is decoded in the current build.
enum class Decoder {
    QtPlugins,   // QImageReader (built-in + imageformats plugins + KImageFormats)
    Svg,         // QSvgRenderer (vector, rendered at device resolution)
    Psd,         // built-in PSD/PSB parser (merged composite + layer info)
    LibRaw,      // camera RAW via libraw
    LibHeif,     // HEIC/HEIF/AVIF via libheif
    Unsupported  // no backend available in this build
};

struct FormatInfo {
    QString     suffix;      // lowercase without dot, e.g. "jpg"
    QString     name;        // human readable, e.g. "JPEG Image"
    Decoder     decoder;
};

class FormatRegistry {
public:
    static FormatRegistry &instance();

    // Lowercase suffix without dot; empty suffix derives from the file name.
    FormatInfo infoFor(const QString &suffix) const;
    Decoder    decoderFor(const QString &suffix) const;
    QString    displayName(const QString &suffix) const;

    // "*.jpg *.png ..." style filter for file dialogs / scanners.
    QString openFileFilter() const;
    const QStringList &supportedSuffixes() const { return m_suffixes; }

    // Formats we would love to open but that need a backend not built in.
    bool isKnownButUnsupported(const QString &suffix) const;

private:
    FormatRegistry();
    void rebuild();

    QList<FormatInfo> m_formats;
    QStringList       m_suffixes;          // all known suffixes
    QStringList       m_unsupportedSuffixes;
};

} // namespace fv
