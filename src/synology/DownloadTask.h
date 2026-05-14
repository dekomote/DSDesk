#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

struct DownloadTask {
    QString taskId;
    QString title;
    QString status;
    QString statusDetail;
    double progress = 0.0;
    double speedDown = 0.0;
    double speedUp = 0.0;
    double sizeTotal = 0.0;
    double sizeDownloaded = 0.0;
    int peersConnected = 0;
    int peersTotal = 0;
    int seedersConnected = 0;
    int seedersTotal = 0;
    int eta = -1;
    QString destination;
    QString uri;
    QString type;
    bool completed = false;
    QJsonObject rawDetail;
    QJsonArray files;
    QJsonArray trackers;

    static DownloadTask fromJson(const QJsonObject &obj)
    {
        DownloadTask task;
        task.taskId = obj["id"].toString();
        task.title = obj["title"].toString();
        task.type = obj["type"].toString();
        task.status = obj["status"].toString();
        task.sizeTotal = obj["size"].toVariant().toDouble();

        auto additional = obj["additional"].toObject();
        auto detail = additional["detail"].toObject();
        task.destination = detail["destination"].toString();
        task.uri = detail["uri"].toString();
        task.statusDetail =
            detail["status_extra"].toObject().isEmpty() ? QString() : QString::fromUtf8(QJsonDocument(detail["status_extra"].toObject()).toJson());
        task.completed = detail["completed"].toBool();

        auto transfer = additional["transfer"].toObject();
        task.speedDown = transfer["speed_download"].toDouble(0);
        task.speedUp = transfer["speed_upload"].toDouble(0);
        task.sizeDownloaded = transfer["size_downloaded"].toDouble(0);

        if (task.sizeTotal > 0)
            task.progress = qBound(0.0, task.sizeDownloaded / task.sizeTotal, 1.0);

        if (task.speedDown > 0 && task.sizeTotal > 0)
            task.eta = static_cast<int>((task.sizeTotal - task.sizeDownloaded) / task.speedDown);
        else
            task.eta = -1;

        auto tracker = additional["tracker"].toArray();
        task.trackers = tracker;

        auto peerInfo = additional["peer"].toObject();
        task.peersConnected = peerInfo["connected_peers"].toInt(0);
        task.peersTotal = peerInfo["total_peers"].toInt(0);
        task.seedersConnected = peerInfo["connected_seeders"].toInt(0);
        task.seedersTotal = peerInfo["total_seeders"].toInt(0);

        task.files = additional["file"].toArray();
        task.rawDetail = obj;

        return task;
    }
};
