#include "DownloadModel.h"
#include "SynologyClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

DownloadModel::DownloadModel(SynologyClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
    , m_pollTimer(new QTimer(this))
{
    connect(m_client, &SynologyClient::tasksReceived, this, &DownloadModel::onTasksReceived);
    connect(m_pollTimer, &QTimer::timeout, this, &DownloadModel::refresh);
    m_pollTimer->setInterval(3000);

    connect(m_client, &SynologyClient::loginSuccess, this, [this]() {
        m_pollTimer->start();
        refresh();
    });
    connect(m_client, &SynologyClient::disconnected, m_pollTimer, &QTimer::stop);
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tasks.size();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.size())
        return {};

    const auto &task = m_tasks[index.row()];

    switch (role) {
    case TaskIdRole:
        return task.taskId;
    case TitleRole:
        return task.title;
    case StatusRole:
        return task.status;
    case StatusDetailRole:
        return task.statusDetail;
    case ProgressRole:
        return task.progress;
    case SpeedDownRole:
        return task.speedDown;
    case SpeedUpRole:
        return task.speedUp;
    case SizeTotalRole:
        return task.sizeTotal;
    case SizeDownloadedRole:
        return task.sizeDownloaded;
    case SizeTotalFormattedRole:
        return formatSize(task.sizeTotal);
    case SizeDownloadedFormattedRole:
        return formatSize(task.sizeDownloaded);
    case SpeedDownFormattedRole:
        return formatSpeed(task.speedDown);
    case SpeedUpFormattedRole:
        return formatSpeed(task.speedUp);
    case EtaRole:
        return task.eta;
    case EtaFormattedRole:
        return formatEta(task.eta);
    case PeersRole:
        return QString("%1/%2").arg(task.peersConnected).arg(task.peersTotal);
    case SeedersRole:
        return QString("%1/%2").arg(task.seedersConnected).arg(task.seedersTotal);
    case TypeRole:
        return task.type;
    case DestinationRole:
        return task.destination;
    case UriRole:
        return task.uri;
    case FilesRole:
        return QVariant::fromValue(task.files.toVariantList());
    case TrackersRole:
        return QVariant::fromValue(task.trackers.toVariantList());
    case RawDetailRole:
        return QString(QJsonDocument(task.rawDetail).toJson(QJsonDocument::Compact));
    default:
        return {};
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    return {
        {TaskIdRole, "taskId"},
        {TitleRole, "title"},
        {StatusRole, "status"},
        {StatusDetailRole, "statusDetail"},
        {ProgressRole, "progress"},
        {SpeedDownRole, "speedDown"},
        {SpeedUpRole, "speedUp"},
        {SizeTotalRole, "sizeTotal"},
        {SizeDownloadedRole, "sizeDownloaded"},
        {SizeTotalFormattedRole, "sizeTotalFormatted"},
        {SizeDownloadedFormattedRole, "sizeDownloadedFormatted"},
        {SpeedDownFormattedRole, "speedDownFormatted"},
        {SpeedUpFormattedRole, "speedUpFormatted"},
        {EtaRole, "eta"},
        {EtaFormattedRole, "etaFormatted"},
        {PeersRole, "peers"},
        {SeedersRole, "seeders"},
        {TypeRole, "type"},
        {DestinationRole, "destination"},
        {UriRole, "uri"},
        {FilesRole, "files"},
        {TrackersRole, "trackers"},
        {RawDetailRole, "rawDetail"},
    };
}

void DownloadModel::refresh()
{
    m_client->refreshTasks();
}

void DownloadModel::onTasksReceived(const QJsonArray &tasks)
{
    QList<DownloadTask> newTasks;
    newTasks.reserve(tasks.size());
    for (const auto &val : tasks)
        newTasks.append(DownloadTask::fromJson(val.toObject()));

    for (int i = 0; i < m_tasks.size(); ++i) {
        bool found = false;
        for (int j = 0; j < newTasks.size(); ++j) {
            if (m_tasks[i].taskId == newTasks[j].taskId) {
                found = true;
                m_tasks[i] = newTasks[j];
                emit dataChanged(index(i), index(i));
                break;
            }
        }
        if (!found) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
            --i;
        }
    }

    for (int i = 0; i < newTasks.size(); ++i) {
        bool found = false;
        for (const auto &t : m_tasks) {
            if (t.taskId == newTasks[i].taskId) {
                found = true;
                break;
            }
        }
        if (!found) {
            beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
            m_tasks.append(newTasks[i]);
            endInsertRows();
        }
    }

    emit countChanged();
}

QString DownloadModel::formatSize(double bytes) const
{
    if (bytes < 1024)
        return QString::number(bytes) + " B";
    double kb = bytes / 1024.0;
    if (kb < 1024)
        return QString::number(kb, 'f', 1) + " KB";
    double mb = kb / 1024.0;
    if (mb < 1024)
        return QString::number(mb, 'f', 2) + " MB";
    double gb = mb / 1024.0;
    return QString::number(gb, 'f', 2) + " GB";
}

QString DownloadModel::formatSpeed(double bytesPerSec) const
{
    if (bytesPerSec < 1024)
        return QString::number(bytesPerSec, 'f', 0) + " B/s";
    double kb = bytesPerSec / 1024.0;
    if (kb < 1024)
        return QString::number(kb, 'f', 1) + " KB/s";
    double mb = kb / 1024.0;
    return QString::number(mb, 'f', 2) + " MB/s";
}

QString DownloadModel::formatEta(int seconds) const
{
    if (seconds < 0)
        return "—";
    if (seconds < 60)
        return QString::number(seconds) + "s";
    if (seconds < 3600)
        return QString("%1m %2s").arg(seconds / 60).arg(seconds % 60);
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    return QString("%1h %2m").arg(h).arg(m);
}
