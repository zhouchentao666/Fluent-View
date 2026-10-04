#include "app/Application.h"
#include "app/ThemeManager.h"
#include "views/MainWindow.h"

#include <QCommandLineParser>
#include <QTimer>

using namespace fv;

int main(int argc, char *argv[])
{
    // High-DPI + rendering tweaks for a crisp Fluent look.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    Application app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Fluent-View - a Fluent Design image viewer"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("files"),
                                 QStringLiteral("Image files to open (multiple allowed)"),
                                 QStringLiteral("[files...]"));
    parser.process(app);

    // Forward to an already-running instance and exit if one exists.
    app.connectServer();
    if (!app.isPrimary())
        return 0;

    app.initTheme();
    app.initTranslator();

    MainWindow window;
    ThemeManager::instance().polishWindow(&window);
    window.show();

    // Open files passed on the command line once the event loop runs.
    const QStringList cliFiles = Application::imageFilesFromCommandLine();
    QTimer::singleShot(0, &window, [&, cliFiles]() {
        if (!cliFiles.isEmpty())
            window.openPaths(cliFiles);
    });

    // Files forwarded from other instances.
    QObject::connect(&app, &Application::filesRequested, &window, &MainWindow::openPaths);

    return app.exec();
}
