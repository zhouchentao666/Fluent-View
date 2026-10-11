cmake_minimum_required(VERSION 3.19)

project(fluent-view
    VERSION 0.1.0
    DESCRIPTION "A cross-platform Fluent Design image viewer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default to Release builds (important for small/fast binaries on CI)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Allow CI to point at a Qt installation
if(DEFINED ENV{QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
endif()

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2)
find_package(Qt6 OPTIONAL_COMPONENTS Shadertools)

qt_standard_project_setup(REQUIRES 6.8)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    src/main.cpp
    src/core/Settings.cpp
    src/core/ImageCache.cpp
    src/core/ImageDecode.cpp
    src/core/ViewerImageProvider.cpp
    src/core/ThumbnailProvider.cpp
    src/core/ImageModel.cpp
    src/core/PlatformUtils.cpp
    src/core/ThemeManager.cpp
    src/core/ImageController.cpp
)

set(RESOURCES
    resources/resources.qrc
)

qt_add_executable(fluent-view WIN32 ${CORE_SOURCES} ${RESOURCES})

# Deterministic output location so packaging steps are the same on every OS.
set_target_properties(fluent-view PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(fluent-view PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(fluent-view PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

# ---------------------------------------------------------------------------
# Platform specifics
# ---------------------------------------------------------------------------
if(WIN32)
    target_compile_definitions(fluent-view PRIVATE
        NOMINMAX
        _USE_MATH_DEFINES
        WIN32_LEAN_AND_MEAN
    )
    # Per-monitor DPI awareness + window controls via application manifest
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.manifest")
        target_sources(fluent-view PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.manifest")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.rc")
        target_sources(fluent-view PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.rc")
    endif()
endif()

if(APPLE)
    set_target_properties(fluent-view PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Fluent View"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS fluent-view
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
