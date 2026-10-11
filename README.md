# Fluent View

A fast, lightweight, cross-platform image viewer built with **Qt 6.8 + Qt Quick (QML)**
using the **Fluent WinUI3** style.

> Target platforms: **Windows x64 · macOS (arm64 / x64) · Linux x64 · Android arm64 · iOS arm64**
>
> Local dev machines need **nothing installed** — every build, package and
> (where possible) signing step runs in **GitHub Actions**.

---

## Features (MVP)

| Area | Status |
|------|--------|
| Open jpg/png/webp/avif/bmp/gif/heic(†) | ✅ |
| Drag & drop / command line / file association | ✅ |
| Previous / next image | ✅ |
| Wheel zoom · double-click fit · fit window · 1:1 · rotate · flip | ✅ |
| Thumbnail sidebar (async) | ✅ |
| Fullscreen / immersive | ✅ |
| Light / dark / system theme | ✅ |
| Info bar: resolution · size · format · path | ✅ |
| Copy to clipboard · set as wallpaper (desktop) · open folder | ✅ |
| Keyboard shortcuts (`j`/`k`/arrows/`space`/`f`/`0`/`1`/`r`/…) | ✅ |
| Settings: theme · zoom mode · remember window · cache limit · thumbnails | ✅ |

† HEIC/HEIF availability depends on platform codec support and the Qt build.

## Performance design

- **Off-GUI-thread decoding.** The main image and thumbnails are served by
  `QQuickAsyncImageProvider`s, so decoding never blocks the UI thread.
- **Memory-bounded.** `Image.sourceSize` is capped to the viewport (× device
  pixel ratio), so even a 100 MP photo is only ever decoded at the resolution
  actually displayed — the GPU texture is what gets scaled. A hard safety cap
  (`MAX_DECODE_SIDE = 8192`) prevents OOM on decode.
- **LRU cache.** Decoded `QImage`s are cached by `ImageCache` (thread-safe LRU)
  with a configurable memory budget (`Settings.maxCacheMB`, default 512 MB).
- **Async thumbnails.** Generated off-thread and cached; the sidebar never stalls.
- **Fast cold start.** A borderless `Window` is shown immediately; the image is
  streamed in afterwards.

## Architecture

```
main.cpp                 app bootstrap, wires C++ core to QML context
src/core/
  Settings              persistent settings (QSettings), exposed to QML
  ImageCache            thread-safe LRU cache for decoded QImages
  ImageDecode           shared off-thread decode (autoTransform, size-capped)
  ViewerImageProvider   async provider for the main image (+ rotation/flip)
  ThumbnailProvider     async provider for thumbnails
  ImageModel            file list + navigation (QAbstractListModel)
  PlatformUtils         clipboard / wallpaper / open-folder (desktop only)
  ThemeManager          applies Qt::ColorScheme (light/dark/system)
  ImageController       current image path + transform + metadata
qml/
  main.qml              frameless window, layout, shortcuts, context menu
  components/           TitleBar, ToolBar, ImageView, ThumbnailSidebar,
                        InfoBar, SettingsDialog, Icon, IconButton, BusyOverlay,
                        ResizeHandles
```

C++ owns all logic (loading / caching / file model); QML owns the UI. No
`QWidget`, no WebEngine / Chromium, no heavy deps — only
`qtbase`, `qtdeclarative`, `qtimageformats`, `qtshadertools`.

## Local build (optional)

Although CI is the intended build path, you can build locally if you have a
matching Qt 6.8 installed:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt6 dir>
cmake --build build
# Windows: windeployqt --qmldir qml build/bin/fluent-view.exe
# macOS:   macdeployqt build/bin/fluent-view.app -dmg
# Linux:   (use linuxdeploy + qt plugin for AppImage / deb)
```

## CI / GitHub Actions

Five workflows live in `.github/workflows/`:

| Workflow | Platform | Output |
|----------|----------|--------|
| `build-windows.yml` | Windows x64 (MSVC) | portable `bin/` (windeployqt) |
| `build-macos.yml`   | macOS universal (arm64+x64) | `.dmg` (macdeployqt) |
| `build-linux.yml`   | Linux x64 | `AppImage` + `.deb` (linuxdeploy) |
| `build-android.yml` | Android arm64 | debug-signed `.apk` (androiddeployqt) |
| `build-ios.yml`     | iOS arm64 | `.app` (unsigned; archive to ship) |

Qt is installed on the runners with [`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action)
(uses aqtinstall). All workflows are triggered on push to `main`, manual
dispatch, and releases.

### Secrets / tuning you may need

- **Android NDK** — `NDK_VERSION` in `build-android.yml` must match the NDK Qt
  was built against (Qt 6.8 → r26b). Adjust if CI reports a mismatch.
- **Android release signing** — for a Play Store AAB, provide a release keystore
  via Secrets and add `--release --sign <keystore> <alias>` to the
  `androiddeployqt` step.
- **iOS signing** — device distribution requires a certificate + provisioning
  profile (GitHub Secrets) and an `xcodebuild -archivePath / -exportArchive`
  step. The workflow currently builds an unsigned `.app`.
- **Windows MSIX** — `packaging/windows/msix/AppxManifest.xml` is provided; build
  with `makeappx` and sign with `signtool` using a code-signing cert from Secrets.

## Project layout

```
CMakeLists.txt          CMake + Qt setup, platform-specific bundling
resources/resources.qrc all QML bundled into the binary
packaging/              per-platform metadata (desktop, plist, manifest, MSIX)
.github/workflows/      the 5 CI pipelines
```

## Non-goals (not in this MVP)

Image editing (crop/filters), cloud sync, accounts, and a plugin system are
explicitly out of scope for now.
