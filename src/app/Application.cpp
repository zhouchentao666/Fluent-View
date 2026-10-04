#include "Application.h"

#include "Settings.h"
#include "ThemeManager.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include <cstring>

namespace fv {

namespace {
const char kPathListHeader[] = "fv-paths:";
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName(QStringLiteral("Fluent-View"));
    setOrganizationName(QStringLiteral("FluentView"));
    setApplicationVersion(QStringLiteral(FV_VERSION));
    setWindowIcon(QIcon(QStringLiteral(":/icons/fluentview.svg")));
}

QString Application::socketName()
{
    // Fixed per-user name; USERNAME-based suffix not needed for a viewer.
    return QStringLiteral("fluent-view-instance");
}

QStringList Application::imageFilesFromCommandLine()
{
    QStringList files;
    const QStringList args = arguments().mid(1);
    for (const QString &a : args) {
        if (a.startsWith(QLatin1Char('-')))
            continue;
        QUrl url(a);
        const QString path = url.isLocalFile() ? url.toLocalFile() : a;
        if (QFileInfo::exists(path))
            files.append(QFileInfo(path).absoluteFilePath());
    }
    return files;
}

void Application::initTheme()
{
    ThemeManager::instance().apply();
}

void Application::initTranslator()
{
    QString lang = Settings::instance().language();
    if (lang == QLatin1String("auto")) {
        const QString name = QLocale::system().name();   // e.g. "zh_CN"
        lang = name.startsWith(QLatin1String("zh")) ? QStringLiteral("zh_CN")
                                                    : QStringLiteral("en");
    }
    if (lang == QLatin1String("en"))
        return;   // source language

    if (m_translator.load(QStringLiteral("fluentview_%1").arg(lang),
                          QStringLiteral(":/i18n"))) {
        installTranslator(&m_translator);
    }
}

void Application::openPaths(const QStringList &paths)
{
    emit filesRequested(paths);
}

void Application::connectServer()
{
    // Try to hand the arguments over to an already-running instance.
    QLocalSocket sock;
    sock.connectToServer(socketName());
    if (sock.waitForConnected(300)) {
        QJsonDocument doc(QJsonArray::fromStringList(imageFilesFromCommandLine()));
        sock.write(kPathListHeader + doc.toJson(QJsonDocument::Compact));
        sock.flush();
        sock.waitForBytesWritten(300);
        sock.disconnectFromServer();
        QTimer::singleShot(0, this, &Application::quit);
        return;
    }

    // We are the primary instance: listen for followers.
    QLocalServer::removeServer(socketName());
    m_server = new QLocalServer(this);
    if (!m_server->listen(socketName()))
        return;   // non-fatal: multi-instance fallback

    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket *c = m_server->nextPendingConnection();
        if (!c)
            return;
        connect(c, &QLocalSocket::disconnected, c, &QLocalSocket::deleteLater);
        connect(c, &QLocalSocket::readyRead, this, [c, this]() {
            const QByteArray data = c->readAll();
            if (!data.startsWith(kPathListHeader))
                return;
            const QByteArray json = data.mid(int(strlen(kPathListHeader)));
            const QStringList paths =
                QJsonDocument::fromJson(json).toVariant().toStringList();
            if (!paths.isEmpty())
                emit filesRequested(paths);
            c->disconnectFromServer();
        });
    });
}

} // namespace fv
