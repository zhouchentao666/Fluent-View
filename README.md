# Fluent-View

A fast, lightweight **Fluent Design image viewer** for Windows, Linux and macOS, built with **Qt 6 / C++17**.

![platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![license](https://img.shields.io/badge/license-MIT-green.svg)

## Highlights

- **Instant startup** - no images on the command line drops you straight into a
  gallery page with recent folders and a lazy-loading thumbnail wall
- **Fluent UI** - dark / light / follow-system themes, Mica + dark titlebar on
  Windows 11, custom frameless title bar, floating pill toolbar
- **Broad format support** - mainstream, professional and niche formats (see
  the table below), including **PSD/PSB** via a built-in parser
- **EXIF inspector** - camera, lens, exposure, ISO, focal length, GPS
- **Slideshow**, fullscreen, rotate/flip, clipboard, wallpaper (Win/macOS), print
- **Multi-instance aware** - opening files from Explorer/Finder reuses the
  running window
- **Languages**: English, 简体中文

## Format support

| Group | Formats | Backend |
|---|---|---|
| Mainstream | JPG/JPEG, PNG/APNG, BMP, GIF (animated), WebP (animated) | Qt image plugins |
| Modern | AVIF, HEIC/HEIF | libheif (optional) |
| Basics | TIFF, ICO, ICNS, SVG (vector) | Qt plugins / QSvgRenderer |
| Pro | **PSD/PSB** (merged preview + layer list), EPS preview, XCF, KRA, ORA, JXL, QOI | Built-in PSD parser / Qt plugins |
| RAW | CR2/CR3/CRW, NEF/NRW, ARW, RAF, ORF, RW2, DNG, PEF, SRW, X3F, 3FR, IIQ, MOS, MRW | libraw (optional) |
| Misc | TGA, DDS, HDR, EXR, PCX, WBMP, SGI | Qt plugins (kimageformats where available) |

Formats without a built-in backend (e.g. PDF first page, ZIP-compressed PSD
composites) show a friendly error page with the file's details instead of a
broken image.

## Building

Requirements: CMake ≥ 3.21, Qt ≥ 6.6 (Core, Gui, Widgets, Concurrent, Svg),
a C++17 compiler. Optional: libraw and libheif for RAW/HEIC support
(auto-detected, or force with `-DFV_WITH_LIBRAW=ON / -DFV_WITH_LIBHEIF=ON`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/fluent-view           # or build/Fluent-View.app on macOS
```

### One-command packaging (CI)

| Platform | Artifacts | Workflow |
|---|---|---|
| Windows x64 + ARM64 | portable `.zip`, `.msix` | `.github/workflows/build-windows.yml` |
| Linux x86_64 | `.AppImage`, `.deb`, `.flatpak` | `.github/workflows/build-linux.yml` |
| Linux ARM64 | `.AppImage` | `.github/workflows/build-linux.yml` |
| macOS Universal | `.dmg` | `.github/workflows/build-macos.yml` |

Push a tag (`v1.0.0`) and all five pipelines run; installers land in a GitHub
Release automatically.

## Command line

```
Fluent-View photo.jpg                 # open one file
Fluent-View a.jpg b.png c.psd         # open several files as a playlist
Fluent-View C:\Pictures               # open a folder in gallery mode
```

## Shortcuts

| Key | Action |
|---|---|
| ← / → | previous / next image (folder traversal) |
| Home / End | first / last image |
| Ctrl+wheel / pinch | zoom at cursor |
| +/- | zoom in / out |
| Ctrl+F / Ctrl+1 | fit window / actual size |
| Ctrl+, / Ctrl+. | rotate left / right |
| Ctrl+T / Ctrl+I | thumbnail bar / info panel |
| Ctrl+G | gallery |
| F5 | slideshow |
| F11 / double-click | fullscreen |
| Space | toggle toolbar in fullscreen |
| Esc | exit slideshow / fullscreen |

## License

MIT - see [LICENSE](LICENSE).
