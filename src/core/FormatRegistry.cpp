#include "FormatRegistry.h"

#include <QImageReader>

#ifdef FV_HAVE_LIBRAW
#  define FV_RAW_DECODER Decoder::LibRaw
#else
#  define FV_RAW_DECODER Decoder::Unsupported
#endif

#ifdef FV_HAVE_LIBHEIF
#  define FV_HEIF_DECODER Decoder::LibHeif
#else
#  define FV_HEIF_DECODER Decoder::Unsupported
#endif

namespace fv {

FormatRegistry &FormatRegistry::instance()
{
    static FormatRegistry reg;
    return reg;
}

FormatRegistry::FormatRegistry()
{
    rebuild();
}

void FormatRegistry::rebuild()
{
    m_formats.clear();

    auto add = [this](const char *suffix, const char *name, Decoder dec) {
        m_formats.append({ QLatin1String(suffix), QLatin1String(name), dec });
    };

    // ---- mainstream -------------------------------------------------------
    add("jpg",   QT_TR_NOOP("JPEG Image"),              Decoder::QtPlugins);
    add("jpeg",  QT_TR_NOOP("JPEG Image"),              Decoder::QtPlugins);
    add("jpe",   QT_TR_NOOP("JPEG Image"),              Decoder::QtPlugins);
    add("jfif",  QT_TR_NOOP("JPEG Image"),              Decoder::QtPlugins);
    add("png",   QT_TR_NOOP("PNG Image"),               Decoder::QtPlugins);
    add("apng",  QT_TR_NOOP("Animated PNG"),            Decoder::QtPlugins);
    add("bmp",   QT_TR_NOOP("Windows Bitmap"),          Decoder::QtPlugins);
    add("dib",   QT_TR_NOOP("Windows Bitmap"),          Decoder::QtPlugins);
    add("gif",   QT_TR_NOOP("GIF Image (animated)"),    Decoder::QtPlugins);
    add("webp",  QT_TR_NOOP("WebP Image"),              Decoder::QtPlugins);
    add("avif",  QT_TR_NOOP("AVIF Image"),              FV_HEIF_DECODER);
    add("heic",  QT_TR_NOOP("HEIC Image"),              FV_HEIF_DECODER);
    add("heif",  QT_TR_NOOP("HEIF Image"),              FV_HEIF_DECODER);
    add("hif",   QT_TR_NOOP("HEIF Image"),              FV_HEIF_DECODER);

    // ---- basics -----------------------------------------------------------
    add("tif",   QT_TR_NOOP("TIFF Image"),              Decoder::QtPlugins);
    add("tiff",  QT_TR_NOOP("TIFF Image"),              Decoder::QtPlugins);
    add("ico",   QT_TR_NOOP("Windows Icon"),            Decoder::QtPlugins);
    add("icns",  QT_TR_NOOP("macOS Icon"),              Decoder::QtPlugins);
    add("svg",   QT_TR_NOOP("SVG Vector Image"),        Decoder::Svg);
    add("svgz",  QT_TR_NOOP("SVG Vector Image"),        Decoder::Svg);
    add("tga",   QT_TR_NOOP("Truevision TGA"),          Decoder::QtPlugins);
    add("dds",   QT_TR_NOOP("DirectDraw Surface"),      Decoder::QtPlugins);
    add("hdr",   QT_TR_NOOP("Radiance HDR"),            Decoder::QtPlugins);
    add("exr",   QT_TR_NOOP("OpenEXR Image"),           Decoder::QtPlugins);
    add("pcx",   QT_TR_NOOP("ZSoft PCX"),               Decoder::QtPlugins);
    add("wbmp",  QT_TR_NOOP("Wireless Bitmap"),         Decoder::QtPlugins);
    add("sgi",   QT_TR_NOOP("Silicon Graphics Image"),  Decoder::QtPlugins);
    add("bw",    QT_TR_NOOP("Silicon Graphics Image"),  Decoder::QtPlugins);
    add("rgba",  QT_TR_NOOP("Silicon Graphics Image"),  Decoder::QtPlugins);
    add("xcf",   QT_TR_NOOP("GIMP XCF Image"),          Decoder::QtPlugins);
    add("psd",   QT_TR_NOOP("Adobe Photoshop Image"),   Decoder::Psd);
    add("psb",   QT_TR_NOOP("Adobe Photoshop Large Document"), Decoder::Psd);
    add("ai",    QT_TR_NOOP("Adobe Illustrator Artwork"), Decoder::QtPlugins);
    add("eps",   QT_TR_NOOP("Encapsulated PostScript"), Decoder::QtPlugins);
    add("pdf",   QT_TR_NOOP("PDF Document (first page)"), Decoder::Unsupported);
    add("kra",   QT_TR_NOOP("Krita Image"),             Decoder::QtPlugins);
    add("ora",   QT_TR_NOOP("OpenRaster Image"),        Decoder::QtPlugins);
    add("qoi",   QT_TR_NOOP("Quite OK Image"),          Decoder::QtPlugins);
    add("jxl",   QT_TR_NOOP("JPEG XL Image"),           Decoder::QtPlugins);
    add("mng",   QT_TR_NOOP("Multiple-image Network Graphics"), Decoder::QtPlugins);
    add("jp2",   QT_TR_NOOP("JPEG 2000"),               Decoder::QtPlugins);
    add("jpg2",  QT_TR_NOOP("JPEG 2000"),               Decoder::QtPlugins);

    // ---- camera RAW -------------------------------------------------------
    add("cr2",   QT_TR_NOOP("Canon RAW 2"),             FV_RAW_DECODER);
    add("cr3",   QT_TR_NOOP("Canon RAW 3"),             FV_RAW_DECODER);
    add("crw",   QT_TR_NOOP("Canon RAW"),               FV_RAW_DECODER);
    add("nef",   QT_TR_NOOP("Nikon Electronic Format"), FV_RAW_DECODER);
    add("nrw",   QT_TR_NOOP("Nikon RAW"),               FV_RAW_DECODER);
    add("arw",   QT_TR_NOOP("Sony Alpha RAW"),          FV_RAW_DECODER);
    add("srf",   QT_TR_NOOP("Sony RAW"),                FV_RAW_DECODER);
    add("sr2",   QT_TR_NOOP("Sony RAW"),                FV_RAW_DECODER);
    add("raf",   QT_TR_NOOP("Fujifilm RAW"),            FV_RAW_DECODER);
    add("orf",   QT_TR_NOOP("Olympus RAW"),             FV_RAW_DECODER);
    add("rw2",   QT_TR_NOOP("Panasonic RAW"),           FV_RAW_DECODER);
    add("raw",   QT_TR_NOOP("Panasonic RAW"),           FV_RAW_DECODER);
    add("dng",   QT_TR_NOOP("Digital Negative"),        FV_RAW_DECODER);
    add("pef",   QT_TR_NOOP("Pentax RAW"),              FV_RAW_DECODER);
    add("srw",   QT_TR_NOOP("Samsung RAW"),             FV_RAW_DECODER);
    add("x3f",   QT_TR_NOOP("Sigma Foveon RAW"),        FV_RAW_DECODER);
    add("3fr",   QT_TR_NOOP("Hasselblad RAW"),          FV_RAW_DECODER);
    add("iiq",   QT_TR_NOOP("Phase One RAW"),           FV_RAW_DECODER);
    add("mos",   QT_TR_NOOP("Leaf RAW"),                FV_RAW_DECODER);
    add("mrw",   QT_TR_NOOP("Minolta RAW"),             FV_RAW_DECODER);

    m_suffixes.clear();
    m_unsupportedSuffixes.clear();
    for (const FormatInfo &f : std::as_const(m_formats)) {
        m_suffixes.append(f.suffix);
        if (f.decoder == Decoder::Unsupported)
            m_unsupportedSuffixes.append(f.suffix);
    }
}

FormatInfo FormatRegistry::infoFor(const QString &suffix) const
{
    const QString s = suffix.toLower().remove(QLatin1Char('.'));
    for (const FormatInfo &f : m_formats)
        if (f.suffix == s)
            return f;
    return { s, QString(), Decoder::QtPlugins };
}

Decoder FormatRegistry::decoderFor(const QString &suffix) const
{
    return infoFor(suffix).decoder;
}

QString FormatRegistry::displayName(const QString &suffix) const
{
    FormatInfo f = infoFor(suffix);
    return f.name.isEmpty() ? f.suffix.toUpper() + QLatin1String(" Image") : f.name;
}

QString FormatRegistry::openFileFilter() const
{
    QStringList stars;
    for (const QString &s : std::as_const(m_suffixes))
        stars.append(QStringLiteral("*.%1").arg(s));
    return QStringLiteral("Images (%1)").arg(stars.join(QLatin1Char(' ')));
}

bool FormatRegistry::isKnownButUnsupported(const QString &suffix) const
{
    return m_unsupportedSuffixes.contains(suffix.toLower().remove(QLatin1Char('.')));
}

} // namespace fv
