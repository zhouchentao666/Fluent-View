#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QCommandLineParser>
#include <QUrl>
#include <QDir>

#include "core/Settings.h"
#include "core/ImageCache.h"
#include "core/ViewerImageProvider.h"
#include "core/ThumbnailProvider.h"
#include "core/ImageModel.h"
#include "core/PlatformUtils.h"
#include "core/ThemeManager.h"
#include "core/ImageController.h"

int main(int argc, char *argv[])
{
    // High-DPI / scaling — important for crisp Fluent UI on Windows & macOS.
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("Fluent View");
    QGuiApplication::setOrganizationName("FluentView");
    QGuiApplication::setOrganizationDomain("fluentview.app");
    QGuiApplication::setApplicationVersion("0.1.0");

    // Fluent WinUI3 style (built into Qt 6.5+).
    QQuickStyle::setStyle("FluentWinUI3");

    QCommandLineParser parser;
    parser.setApplicationDescription("Fluent View — cross-platform Fluent Design image viewer");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Image file(s) or folder to open", "[files...]");
    parser.process(app);

    // --- Core objects -------------------------------------------------------
    auto *settings = new Settings(&app);
    auto *theme = new ThemeManager(settings, &app);
    theme->apply();

    auto *cache = new ImageCache(&app);
    cache->setMaxBytes(qint64(settings->maxCacheMB()) * 1024 * 1024);

    auto *model = new ImageModel(&app);
    auto *controller = new ImageController(&app);
    auto *platform = new PlatformUtils(&app);

    // Keep the current-image controller in sync with the model selection.
    QObject::connect(model, &ImageModel::currentIndexChanged, &app, [model, controller]() {
        controller->setPath(model->currentPath());
    });
    // When the cache budget changes, propagate to the cache.
    QObject::connect(settings, &Settings::maxCacheMBChanged, &app, [settings, cache]() {
        cache->setMaxBytes(qint64(settings->maxCacheMB()) * 1024 * 1024);
    });

    // --- Image providers (async, off GUI thread) ----------------------------
    auto *viewerProvider = new ViewerImageProvider(cache);
    auto *thumbProvider = new ThumbnailProvider(cache);

    QQmlApplicationEngine engine;
    engine.addImageProvider("viewer", viewerProvider);
    engine.addImageProvider("thumb", thumbProvider);

    engine.rootContext()->setContextProperty("Settings", settings);
    engine.rootContext()->setContextProperty("ThemeManager", theme);
    engine.rootContext()->setContextProperty("ImageModel", model);
    engine.rootContext()->setContextProperty("ImageCtl", controller);
    engine.rootContext()->setContextProperty("Platform", platform);

    // --- Open files passed on the command line / via file association -------
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QStringList paths;
        for (const QString &a : args)
            paths.append(QDir::toNativeSeparators(QUrl::fromUserInput(a).toLocalFile()));
        // A single file navigates its whole folder; several explicit files
        // keep exactly that list.
        if (paths.size() == 1)
            model->openPath(paths.at(0));
        else
            model->setFiles(paths);
    }

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &loaded) {
            if (!obj && loaded == url)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
