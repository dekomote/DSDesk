#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSslConfiguration>
#include <QTimer>
#include <QUrl>
#include <functional>

class SynologyClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString defaultDestination READ defaultDestination NOTIFY defaultDestinationChanged)

public:
    explicit SynologyClient(QObject *parent = nullptr);
    ~SynologyClient() override = default;

    Q_INVOKABLE void login(const QString &host, quint16 port, const QString &username, const QString &password, bool useSsl, bool trustCert);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void refreshTasks();
    Q_INVOKABLE void createTaskUrl(const QString &url, const QString &destination);
    Q_INVOKABLE void createTaskTorrent(const QUrl &torrentPath, const QString &destination);
    Q_INVOKABLE void pauseTask(const QString &taskId);
    Q_INVOKABLE void resumeTask(const QString &taskId);
    Q_INVOKABLE void deleteTask(const QString &taskId, bool forceComplete = false);
    Q_INVOKABLE void getTaskDetail(const QString &taskId);
    Q_INVOKABLE void getDownloadStationConfig();

    bool isConnected() const;
    QString connectionState() const;
    QString defaultDestination() const;

signals:
    void connectedChanged();
    void connectionStateChanged();
    void defaultDestinationChanged();
    void loginSuccess();
    void loginFailed(const QString &errorMessage);
    void disconnected();
    void tasksReceived(const QJsonArray &tasks);
    void downloadStationConfigReceived(const QString &defaultDestination);
    void taskDetailReceived(const QJsonObject &detail);
    void taskCreated(const QString &taskId);
    void taskCreateFailed(const QString &errorMessage);
    void networkError(const QString &errorMessage);

private slots:
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    void doLogin();
    void post(const QString &apiName,
              const QString &path,
              const QJsonObject &params,
              std::function<void(const QJsonObject &)> onSuccess,
              std::function<void(const QString &)> onError);
    QString apiUrl(const QString &path) const;
    void get(const QString &apiName, const QString &path, const QJsonObject &params, std::function<void(const QJsonObject &)> onSuccess);
    void get(const QString &apiName,
             const QString &path,
             const QJsonObject &params,
             std::function<void(const QJsonObject &)> onSuccess,
             std::function<void(const QString &)> onError);
    void post(const QString &apiName, const QString &path, const QJsonObject &params, std::function<void(const QJsonObject &)> callback);
    void postMultipart(const QString &apiName, const QString &path, QHttpMultiPart *multiPart, std::function<void(const QJsonObject &)> callback);
    void handleApiResponse(QNetworkReply *reply, std::function<void(const QJsonObject &)> callback);
    void handleApiResponse(QNetworkReply *reply, std::function<void(const QJsonObject &)> onSuccess, std::function<void(const QString &)> onError);
    QString handleNetworkError(QNetworkReply *reply);
    void discoverApis();

    QNetworkAccessManager *m_nam;
    QString m_host;
    quint16 m_port = 5000;
    QString m_username;
    QString m_password;
    bool m_useSsl = false;
    bool m_trustCert = false;
    bool m_connected = false;
    QString m_sid;
    QString m_synoToken;
    QString m_connectionState;
    QString m_defaultDestination;

    struct ApiInfo {
        QString path;
        int maxVersion = 1;
    };
    QHash<QString, ApiInfo> m_apis;
    bool m_apisDiscovered = false;
};
