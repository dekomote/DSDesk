#include "SynologyClient.h"
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QUrlQuery>

SynologyClient::SynologyClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::sslErrors, this, &SynologyClient::onSslErrors);
}

QString SynologyClient::apiUrl(const QString &path) const
{
    return QString("%1://%2:%3/webapi/%4").arg(m_useSsl ? "https" : "http").arg(m_host).arg(m_port).arg(path);
}

bool SynologyClient::isConnected() const
{
    return m_connected;
}
QString SynologyClient::connectionState() const
{
    return m_connectionState;
}
QString SynologyClient::defaultDestination() const
{
    return m_defaultDestination;
}

void SynologyClient::login(const QString &host, quint16 port, const QString &username, const QString &password, bool useSsl, bool trustCert)
{
    if (m_connected)
        logout();

    m_host = host;
    m_port = port;
    m_username = username;
    m_password = password;
    m_useSsl = useSsl;
    m_trustCert = trustCert;
    m_connectionState = tr("Connecting...");
    emit connectionStateChanged();

    m_apis.clear();
    m_apisDiscovered = false;

    QJsonObject params;
    params["version"] = "1";
    params["method"] = "query";
    params["query"] =
        "SYNO.DownloadStation.Info,SYNO.DownloadStation.Task,"
        "SYNO.DownloadStation2.Task,SYNO.API.Auth";

    get("SYNO.API.Info", "query.cgi", params, [this](const QJsonObject &resp) {
        auto data = resp["data"].toObject();
        for (auto it = data.begin(); it != data.end(); ++it) {
            ApiInfo apiInfo;
            apiInfo.path = it.value().toObject()["path"].toString();
            apiInfo.maxVersion = it.value().toObject()["maxVersion"].toInt(1);
            m_apis[it.key()] = apiInfo;
        }
        m_apisDiscovered = true;
        doLogin();
    });
}

void SynologyClient::doLogin()
{
    QJsonObject params;
    params["version"] = "3";
    params["method"] = "login";
    params["account"] = m_username;
    params["passwd"] = m_password;
    params["session"] = "DownloadStation";
    params["format"] = "cookie";
    params["enable_syno_token"] = "yes";
    params["client"] = "DSDesk";

    post("SYNO.API.Auth", "auth.cgi", params, [this](const QJsonObject &resp) {
        m_sid = resp["data"].toObject()["sid"].toString();
        m_synoToken = resp["data"].toObject()["synotoken"].toString();

        if (m_sid.isEmpty()) {
            m_connectionState = tr("Login failed");
            emit connectionStateChanged();
            emit loginFailed(tr("Invalid credentials"));
            return;
        }

        m_connected = true;
        m_connectionState = tr("Connected");
        m_password = "";
        emit connectedChanged();
        emit connectionStateChanged();
        emit loginSuccess();
        refreshTasks();
    });
}

void SynologyClient::logout()
{
    if (!m_connected)
        return;

    QJsonObject params;
    params["version"] = "3";
    params["method"] = "logout";
    params["session"] = "DownloadStation";

    post("SYNO.API.Auth", "auth.cgi", params, [this](const QJsonObject &) {
        m_sid.clear();
        m_synoToken.clear();
        m_connected = false;
        m_connectionState = tr("Disconnected");
        m_apis.clear();
        m_apisDiscovered = false;
        emit connectedChanged();
        emit connectionStateChanged();
        emit disconnected();
    });
}

void SynologyClient::refreshTasks()
{
    if (!m_connected || !m_apis.contains("SYNO.DownloadStation.Task"))
        return;

    auto info = m_apis["SYNO.DownloadStation.Task"];

    QJsonObject params;
    params["version"] = QString::number(info.maxVersion);
    params["method"] = "list";
    params["offset"] = "0";
    params["limit"] = "-1";
    params["additional"] = "detail,transfer,file,tracker";

    get("SYNO.DownloadStation.Task", info.path, params, [this](const QJsonObject &resp) {
        auto tasks = resp["data"].toObject()["tasks"].toArray();
        emit tasksReceived(tasks);
    });
}

void SynologyClient::createTaskUrl(const QString &url, const QString &destination)
{
    const QString api = "SYNO.DownloadStation.Task";

    if (!m_apis.contains(api)) {
        emit taskCreateFailed(tr("API not discovered"));
        return;
    }
    auto info = m_apis[api];

    QJsonObject params;
    params["api"] = api;
    params["_sid"] = m_sid;
    params["version"] = "2";
    params["method"] = "create";
    params["uri"] = url;

    if (!destination.isEmpty())
        params["destination"] = destination;

    QUrl fullUrl(apiUrl(info.path));

    QNetworkRequest req(fullUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    if (!m_synoToken.isEmpty())
        req.setRawHeader("X-SYNO-TOKEN", m_synoToken.toUtf8());

    QByteArray body;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (!body.isEmpty())
            body.append('&');
        body.append(QUrl::toPercentEncoding(it.key()) + '=' + QUrl::toPercentEncoding(it.value().toString()));
    }

    QNetworkReply *reply = m_nam->post(req, body);
    handleApiResponse(
        reply,
        [this](const QJsonObject &) {
            emit taskCreated(QString());
            refreshTasks();
        },
        [this](const QString &msg) {
            emit taskCreateFailed(msg);
        });
}

void SynologyClient::createTaskTorrent(const QUrl &torrentPath, const QString &destination)
{
    const auto api = "SYNO.DownloadStation2.Task";

    if (!m_apis.contains(api)) {
        emit taskCreateFailed(tr("API not discovered"));
        return;
    }
    auto info = m_apis[api];

    QUrl fullUrl(apiUrl(info.path));
    QString localPath = torrentPath.toLocalFile();

    if (localPath.isEmpty()) {
        emit taskCreateFailed(tr("Invalid file path"));
        return;
    }

    QFile *file = new QFile(localPath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        emit taskCreateFailed(tr("Cannot open file: %1").arg(localPath));
        return;
    }

    QString fileParamId = QString::number(QRandomGenerator::global()->generate());
    QString fileName = QFileInfo(localPath).fileName();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    auto addPart = [&](const QString &name, const QByteArray &value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"%1\"").arg(name));
        part.setBody(value);
        multiPart->append(part);
    };

    addPart("api", api);
    addPart("method", "create");
    addPart("version", QByteArray::number(info.maxVersion));
    addPart("file", ("[\"" + fileParamId + "\"]").toUtf8());
    addPart("type", "\"file\"");
    addPart("create_list", "false");

    if (!destination.isEmpty())
        addPart("destination", ("\"" + destination + "\"").toUtf8());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"%1\"; filename=\"%2\"").arg(fileParamId, fileName));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QUrlQuery query;
    if (!m_sid.isEmpty())
        query.addQueryItem("_sid", m_sid);
    fullUrl.setQuery(query);

    QNetworkRequest req(fullUrl);
    req.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multiPart->boundary());
    if (!m_synoToken.isEmpty())
        req.setRawHeader("X-SYNO-TOKEN", m_synoToken.toUtf8());

    QNetworkReply *reply = m_nam->post(req, multiPart);
    multiPart->setParent(reply);
    handleApiResponse(
        reply,
        [this](const QJsonObject &) {
            emit taskCreated(QString());
            refreshTasks();
        },
        [this](const QString &msg) {
            emit taskCreateFailed(msg);
        });
}

void SynologyClient::pauseTask(const QString &taskId)
{
    if (!m_apis.contains("SYNO.DownloadStation.Task"))
        return;
    auto info = m_apis["SYNO.DownloadStation.Task"];

    QJsonObject params;
    params["version"] = QString::number(info.maxVersion);
    params["method"] = "pause";
    params["id"] = taskId;

    get("SYNO.DownloadStation.Task", info.path, params, [this](const QJsonObject &) {
        refreshTasks();
    });
}

void SynologyClient::resumeTask(const QString &taskId)
{
    if (!m_apis.contains("SYNO.DownloadStation.Task"))
        return;
    auto info = m_apis["SYNO.DownloadStation.Task"];

    QJsonObject params;
    params["version"] = QString::number(info.maxVersion);
    params["method"] = "resume";
    params["id"] = taskId;

    get("SYNO.DownloadStation.Task", info.path, params, [this](const QJsonObject &) {
        refreshTasks();
    });
}

void SynologyClient::deleteTask(const QString &taskId, bool forceComplete)
{
    if (!m_apis.contains("SYNO.DownloadStation.Task"))
        return;
    auto info = m_apis["SYNO.DownloadStation.Task"];

    QJsonObject params;
    params["version"] = QString::number(info.maxVersion);
    params["method"] = "delete";
    params["id"] = taskId;
    params["force_complete"] = forceComplete ? "true" : "false";

    get("SYNO.DownloadStation.Task", info.path, params, [this](const QJsonObject &) {
        refreshTasks();
    });
}

void SynologyClient::getTaskDetail(const QString &taskId)
{
    if (!m_apis.contains("SYNO.DownloadStation.Task"))
        return;
    auto info = m_apis["SYNO.DownloadStation.Task"];

    QJsonObject params;
    params["version"] = "1";
    params["method"] = "getinfo";
    params["id"] = taskId;
    params["additional"] = "detail,transfer,file,tracker,peer";

    get("SYNO.DownloadStation.Task", info.path, params, [this](const QJsonObject &resp) {
        auto tasks = resp["data"].toObject()["tasks"].toArray();
        if (tasks.isEmpty())
            return;
        auto task = tasks.at(0).toObject();
        emit taskDetailReceived(task);
    });
}

void SynologyClient::getDownloadStationConfig()
{
    if (!m_apis.contains("SYNO.DownloadStation.Info"))
        return;
    auto info = m_apis["SYNO.DownloadStation.Info"];

    QJsonObject params;
    params["version"] = "1";
    params["method"] = "getconfig";

    get("SYNO.DownloadStation.Info", info.path, params, [this](const QJsonObject &resp) {
        auto config = resp["data"].toObject();
        QString defaultDest = config["default_destination"].toString();
        emit downloadStationConfigReceived(defaultDest);
    });
}

void SynologyClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    if (m_trustCert) {
        reply->ignoreSslErrors(errors);
    } else {
        for (const auto &error : errors)
            qWarning() << "SSL error:" << error.errorString();
    }
}

QString SynologyClient::handleNetworkError(QNetworkReply *reply)
{
    QString errStr = reply->errorString();
    if (reply->error() == QNetworkReply::ConnectionRefusedError)
        errStr = tr("Connection refused");
    else if (reply->error() == QNetworkReply::HostNotFoundError)
        errStr = tr("Host not found");
    else if (reply->error() == QNetworkReply::TimeoutError)
        errStr = tr("Connection timed out");
    else if (reply->error() == QNetworkReply::SslHandshakeFailedError)
        errStr = tr("SSL handshake failed");

    if (!m_connected && !m_sid.isEmpty()) {
        m_connectionState = errStr;
        emit connectionStateChanged();
    }
    return errStr;
}

void SynologyClient::get(const QString &apiName, const QString &path, const QJsonObject &params, std::function<void(const QJsonObject &)> onSuccess)
{
    get(apiName, path, params, onSuccess, [this](const QString &msg) {
        emit networkError(msg);
    });
}

void SynologyClient::get(const QString &apiName,
                         const QString &path,
                         const QJsonObject &params,
                         std::function<void(const QJsonObject &)> onSuccess,
                         std::function<void(const QString &)> onError)
{
    QUrl url(apiUrl(path));

    QByteArray query = "api=" + QUrl::toPercentEncoding(apiName);
    if (!m_sid.isEmpty())
        query += "&_sid=" + QUrl::toPercentEncoding(m_sid);
    for (auto it = params.begin(); it != params.end(); ++it)
        query += '&' + QUrl::toPercentEncoding(it.key()) + '=' + QUrl::toPercentEncoding(it.value().toString());
    url.setQuery(QString::fromUtf8(query));

    QNetworkRequest req(url);
    if (!m_synoToken.isEmpty())
        req.setRawHeader("X-SYNO-TOKEN", m_synoToken.toUtf8());

    QNetworkReply *reply = m_nam->get(req);
    handleApiResponse(reply, onSuccess, onError);
}

void SynologyClient::post(const QString &apiName, const QString &path, const QJsonObject &params, std::function<void(const QJsonObject &)> callback)
{
    post(apiName, path, params, callback, [this](const QString &msg) {
        emit networkError(msg);
    });
}

void SynologyClient::post(const QString &apiName,
                          const QString &path,
                          const QJsonObject &params,
                          std::function<void(const QJsonObject &)> onSuccess,
                          std::function<void(const QString &)> onError)
{
    QUrl url(apiUrl(path));

    QUrlQuery query;
    query.addQueryItem("api", apiName);
    if (!m_sid.isEmpty())
        query.addQueryItem("_sid", m_sid);
    url.setQuery(query);

    QNetworkRequest req(url);
    if (!m_synoToken.isEmpty())
        req.setRawHeader("X-SYNO-TOKEN", m_synoToken.toUtf8());

    QByteArray body;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (!body.isEmpty())
            body.append('&');
        body.append(QUrl::toPercentEncoding(it.key()) + '=' + QUrl::toPercentEncoding(it.value().toString()));
    }

    QNetworkReply *reply = m_nam->post(req, body);
    handleApiResponse(reply, onSuccess, onError);
}

void SynologyClient::handleApiResponse(QNetworkReply *reply, std::function<void(const QJsonObject &)> callback)
{
    handleApiResponse(reply, callback, [this](const QString &msg) {
        emit networkError(msg);
    });
}

void SynologyClient::handleApiResponse(QNetworkReply *reply, std::function<void(const QJsonObject &)> onSuccess, std::function<void(const QString &)> onError)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess, onError]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            onError(handleNetworkError(reply));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            onError(tr("Invalid JSON response"));
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj["success"].toBool()) {
            int code = obj["error"].toObject()["code"].toInt();
            onError(tr("API error %1").arg(code));
            return;
        }

        onSuccess(obj);
    });
}
