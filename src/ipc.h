#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>

class IpcServer : public QObject
{
    Q_OBJECT
public:
    explicit IpcServer(const QString &serverName, QObject *parent = nullptr)
        : QObject(parent)
        , m_serverName(serverName)
        , m_server(new QLocalServer(this))
    {
    }

    bool hasExistingInstance()
    {
        QLocalSocket socket;
        socket.connectToServer(m_serverName);
        return socket.waitForConnected(200);
    }

    bool sendTorrent(const QString &torrent)
    {
        QLocalSocket socket;
        socket.connectToServer(m_serverName);
        if (socket.waitForConnected(200)) {
            socket.write(torrent.toUtf8());
            socket.waitForBytesWritten(500);
            return true;
        }
        return false;
    }

    void startListening()
    {
        QLocalServer::removeServer(m_serverName);
        m_server->listen(m_serverName);
        connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
    }

signals:
    void torrentReceived(const QString &torrent);

private slots:
    void onNewConnection()
    {
        while (m_server->hasPendingConnections()) {
            QLocalSocket *conn = m_server->nextPendingConnection();
            connect(conn, &QLocalSocket::readyRead, this, [this, conn]() {
                QByteArray data = conn->readAll();
                if (!data.isEmpty())
                    emit torrentReceived(QString::fromUtf8(data));
            });
            connect(conn, &QLocalSocket::disconnected, conn, &QObject::deleteLater);
        }
    }

private:
    QString m_serverName;
    QLocalServer *m_server;
};
