#!/usr/bin/env bash
# Generates packaging/macos/fluentview.icns from resources/icons/fluentview.svg.
# Requires: rsvg-convert or qlmanage or ImageMagick (any one), and macOS iconutil.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SVG="$ROOT/resources/icons/fluentview.svg"
ICONSET="$(mktemp -d)/fluentview.iconset"
mkdir -p "$ICONSET"

render() {
    local size="$1" name="$2"
    if command -v magick >/dev/null 2>&1; then
        magick "$SVG" -resize "${size}x${size}" "$ICONSET/$name.png"
    elif command -v rsvg-convert >/dev/null 2>&1; then
        rsvg-convert -w "$size" -h "$size" "$SVG" -o "$ICONSET/$name.png"
    else
        echo "No SVG rasterizer found (magick / rsvg-convert)." >&2
        exit 1
    fi
}

render 16   icon_16x16
render 32   icon_16x16@2x
render 32   icon_32x32
render 64   icon_32x32@2x
render 128  icon_128x128
render 256  icon_128x128@2x
render 256  icon_256x256
render 512  icon_256x256@2x
render 512  icon_512x512
render 1024 icon_512x512@2x

OUT="$ROOT/packaging/macos/fluentview.icns"
iconutil -c icns "$ICONSET" -o "$OUT"
echo "ICNS written: $OUT"
