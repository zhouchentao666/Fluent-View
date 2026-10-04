#pragma once

#include <QApplication>
#include <QTranslator>

class QLocalServer;

namespace fv {

class Application : public QApplication {
    Q_OBJECT
public:
    Application(int &argc, char **argv);

    // True when this instance is the primary one; false when another instance
    // already runs (this process will exit after forwarding its arguments).
    bool isPrimary() const { return m_server != nullptr; }

    void initTheme();
    void initTranslator();
    void connectServer();

    static QStringList imageFilesFromCommandLine();

public slots:
    void openPaths(const QStringList &paths);

signals:
    void filesRequested(const QStringList &paths);

private:
    void startServer();
    static QString socketName();

    QLocalServer *m_server = nullptr;
    QTranslator   m_translator;
    QTranslator   m_qtTranslator;
};

} // namespace fv
