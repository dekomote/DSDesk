#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTranslator>
#include <QUrl>

#include "ipc.h"
#include "synology/DownloadModel.h"
#include "synology/SynologyClient.h"
#include "synology/SynologySettings.h"

static QTranslator s_appTranslator;
static QTranslator s_qtTranslator;

static void loadLanguage()
{
    QGuiApplication::removeTranslator(&s_appTranslator);
    QGuiApplication::removeTranslator(&s_qtTranslator);

    QString locale = QLocale::system().name().left(2);

    if (locale == "en")
        return;

    if (s_appTranslator.load(":/translations/DSDesk_" + locale))
        QGuiApplication::installTranslator(&s_appTranslator);

    if (s_qtTranslator.load(":/translations/qt_" + locale))
        QGuiApplication::installTranslator(&s_qtTranslator);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(APP_NAME_STRING);
    app.setOrganizationName(APP_ORG_STRING);
    app.setApplicationVersion(APP_VERSION_STRING);
    app.setWindowIcon(QIcon(":/icons/com.dekomote.dsdesk.svg"));

    QQuickStyle::setStyle("Material");

    // Parse torrent/magnet argument
    QString pendingTorrent;
    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (QUrl(arg).isLocalFile()) {
            pendingTorrent = QUrl::fromLocalFile(arg).toString();
        } else {
            pendingTorrent = arg;
        }
        break;
    }

    // Create IPC but don't start listening yet
    IpcServer *ipc = new IpcServer(app.applicationName(), &app);

    // Check for existing instance BEFORE we start listening
    if (ipc->hasExistingInstance()) {
        if (!pendingTorrent.isEmpty())
            ipc->sendTorrent(pendingTorrent);
        return 0;
    }

    // No existing instance, start listening and emit initial torrent if present
    ipc->startListening();

    SynologySettings settings;
    SynologyClient client;
    DownloadModel model(&client);

    loadLanguage();

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("synoClient", &client);
    engine.rootContext()->setContextProperty("synoSettings", &settings);
    engine.rootContext()->setContextProperty("downloadModel", &model);
    engine.rootContext()->setContextProperty("ipc", ipc);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QGuiApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("DSDesk", "Main");

    // Emit initial torrent after QML is loaded
    if (!pendingTorrent.isEmpty())
        emit ipc->torrentReceived(pendingTorrent);

    return app.exec();
}
