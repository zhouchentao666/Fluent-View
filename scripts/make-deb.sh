#!/usr/bin/env bash
# Builds a Debian package from a staged install tree.
# Usage: scripts/make-deb.sh <install-dir> <output-dir> <version> [arch]
set -euo pipefail

INSTALL_DIR="${1:?install dir required}"
OUT_DIR="${2:?output dir required}"
VERSION="${3:?version required}"
ARCH="${4:-amd64}"

PKGROOT="$(mktemp -d)"
trap 'rm -rf "$PKGROOT"' EXIT

# Payload
mkdir -p "$PKGROOT/usr/bin"
cp -r "$INSTALL_DIR"/usr/* "$PKGROOT/usr/" 2>/dev/null || cp -r "$INSTALL_DIR"/* "$PKGROOT/usr/"
chmod +x "$PKGROOT/usr/bin/fluent-view"

# Control
mkdir -p "$PKGROOT/DEBIAN"
cat > "$PKGROOT/DEBIAN/control" <<EOF
Package: fluent-view
Version: $VERSION
Architecture: $ARCH
Maintainer: Fluent-View Contributors <noreply@fluentview.invalid>
Installed-Size: $(du -sk "$PKGROOT/usr" | cut -f1)
Depends: libqt6core6t64 (>= 6.4) | libqt6core6 (>= 6.4), libqt6gui6 (>= 6.4), libqt6widgets6 (>= 6.4), libqt6svg6 (>= 6.4), libqt6concurrent6 (>= 6.4), libgl1, libglib2.0-0t64 | libglib2.0-0
Section: graphics
Priority: optional
Homepage: https://github.com/fluent-view/fluent-view
Description: Fluent Design image viewer
 A fast, lightweight image viewer with a Fluent Design interface.
 Supports JPEG/PNG/GIF/WebP/AVIF/HEIC/TIFF/SVG, PSD/PSB (merged preview +
 layer info), camera RAW via optional libraw, slideshow, EXIF inspector,
 thumbnail gallery and multi-language UI (English, Simplified Chinese).
EOF

mkdir -p "$OUT_DIR"
dpkg-deb --build --root-owner-group "$PKGROOT" \
    "$OUT_DIR/fluent-view_${VERSION}_${ARCH}.deb"

echo "DEB written: $OUT_DIR/fluent-view_${VERSION}_${ARCH}.deb"
