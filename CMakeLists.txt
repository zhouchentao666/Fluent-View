cmake_minimum_required(VERSION 3.21)

project(FluentView
    VERSION 1.0.0
    DESCRIPTION "Fluent-View - a Fluent Design image viewer"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg)
find_package(Qt6 QUIET COMPONENTS LinguistTools)

qt_standard_project_setup(REQUIRES 6.6)

# ---------------------------------------------------------------------------
# Optional codec libraries (auto-detected; force with ON/OFF)
#   FV_WITH_LIBRAW  -> camera RAW (CR2/CR3/NEF/ARW/RAF/ORF/RW2/DNG)
#   FV_WITH_LIBHEIF -> HEIC/HEIF + AVIF fallback decoder
# ---------------------------------------------------------------------------
find_library(FV_LIBRAW_LIBRARY NAMES raw libraw)
find_path(FV_LIBRAW_INCLUDE_DIR libraw/libraw.h)
find_library(FV_LIBHEIF_LIBRARY NAMES heif libheif)
find_path(FV_LIBHEIF_INCLUDE_DIR libheif/heif.h)

include(CMakeDependentOption)
include(FeatureSummary)

macro(fv_optional pkg found_var libvar incvar define)
    if(${pkg} STREQUAL "AUTO")
        if(${libvar} AND ${incvar})
            set(${define} ON)
        else()
            set(${define} OFF)
        endif()
    elseif(${pkg})
        if(NOT (${libvar} AND ${incvar}))
            message(FATAL_ERROR "${define} is ON but ${libvar}/${incvar} were not found")
        endif()
        set(${define} ON)
    else()
        set(${define} OFF)
    endif()
endmacro()

fv_optional(FV_WITH_LIBRAW FV_LIBRAW FV_LIBRAW_LIBRARY FV_LIBRAW_INCLUDE_DIR FV_HAVE_LIBRAW)
fv_optional(FV_WITH_LIBHEIF FV_LIBHEIF FV_LIBHEIF_LIBRARY FV_LIBHEIF_INCLUDE_DIR FV_HAVE_LIBHEIF)

add_feature_info(LibRaw   FV_HAVE_LIBRAW   "camera RAW decoding (CR2/CR3/NEF/ARW/RAF/ORF/RW2/DNG)")
add_feature_info(LibHeif  FV_HAVE_LIBHEIF  "HEIC/HEIF/AVIF decoding via libheif")

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(FV_SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/Settings.cpp
    src/app/ThemeManager.cpp
    src/core/FormatRegistry.cpp
    src/core/ExifParser.cpp
    src/core/PsdParser.cpp
    src/core/ImageLoader.cpp
    src/core/FolderScanner.cpp
    src/core/ThumbnailCache.cpp
    src/platform/PlatformIntegration.cpp
    src/views/MainWindow.cpp
    src/views/ImageView.cpp
    src/views/TitleBar.cpp
    src/views/FloatingToolbar.cpp
    src/views/InfoPanel.cpp
    src/views/ThumbnailBar.cpp
    src/views/GalleryModel.cpp
    src/views/GalleryPage.cpp
)

set(FV_HEADERS
    src/app/Application.h
    src/app/Settings.h
    src/app/ThemeManager.h
    src/core/FormatRegistry.h
    src/core/ImageTypes.h
    src/core/ExifParser.h
    src/core/PsdParser.h
    src/core/ImageLoader.h
    src/core/FolderScanner.h
    src/core/ThumbnailCache.h
    src/platform/PlatformIntegration.h
    src/views/MainWindow.h
    src/views/ImageView.h
    src/views/TitleBar.h
    src/views/FloatingToolbar.h
    src/views/InfoPanel.h
    src/views/ThumbnailBar.h
    src/views/GalleryModel.h
    src/views/GalleryPage.h
)

if(WIN32)
    # Executable icon + version resource (icon is generated in CI from the SVG)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/fluentview.ico")
        set(FV_ICON_ABS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/fluentview.ico")
        configure_file(packaging/windows/app.rc.in
                       "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
        list(APPEND FV_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    endif()
endif()

qt_add_executable(fluent-view WIN32 MACOSX_BUNDLE
    ${FV_SOURCES} ${FV_HEADERS}
    resources/resources.qrc
)

target_compile_definitions(fluent-view PRIVATE
    FV_VERSION="${PROJECT_VERSION}"
    $<$<BOOL:${FV_HAVE_LIBRAW}>:FV_HAVE_LIBRAW>
    $<$<BOOL:${FV_HAVE_LIBHEIF}>:FV_HAVE_LIBHEIF>
)

target_include_directories(fluent-view PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $<$<BOOL:${FV_HAVE_LIBRAW}>:${FV_LIBRAW_INCLUDE_DIR}>
    $<$<BOOL:${FV_HAVE_LIBHEIF}>:${FV_LIBHEIF_INCLUDE_DIR}>
)

target_link_libraries(fluent-view PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent Qt6::Svg
    $<$<BOOL:${FV_HAVE_LIBRAW}>:${FV_LIBRAW_LIBRARY}>
    $<$<BOOL:${FV_HAVE_LIBHEIF}>:${FV_LIBHEIF_LIBRARY}>
)

if(WIN32)
    target_link_libraries(fluent-view PRIVATE dwmapi user32 shell32 gdi32)
    target_compile_definitions(fluent-view PRIVATE UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(fluent-view PRIVATE /utf-8 /permissive- /Zc:__cplusplus)
    endif()
endif()

if(APPLE)
    set_target_properties(fluent-view PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Fluent-View"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.fluentview.FluentView"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "fluentview.icns"
        OUTPUT_NAME "Fluent-View"
    )
endif()

# ---------------------------------------------------------------------------
# Translations (.ts -> .qm embedded into the binary resources)
# ---------------------------------------------------------------------------
if(TARGET Qt6::lrelease)
    qt_add_translations(fluent-view
        TS_FILES
            resources/i18n/fluentview_zh_CN.ts
            resources/i18n/fluentview_en.ts
    )
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

if(WIN32)
    install(TARGETS fluent-view RUNTIME DESTINATION .)
else()
    install(TARGETS fluent-view
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(NOT APPLE)
    install(FILES packaging/linux/com.fluentview.FluentView.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/com.fluentview.FluentView.metainfo.xml
            DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
            RENAME com.fluentview.FluentView.appdata.xml)
    install(FILES resources/icons/fluentview.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME com.fluentview.FluentView.svg)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
