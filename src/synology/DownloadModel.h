#pragma once

#include "DownloadTask.h"
#include <QAbstractListModel>
#include <QJsonArray>
#include <QTimer>

class SynologyClient;

class DownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        TaskIdRole = Qt::UserRole + 1,
        TitleRole,
        StatusRole,
        StatusDetailRole,
        ProgressRole,
        SpeedDownRole,
        SpeedUpRole,
        SizeTotalRole,
        SizeDownloadedRole,
        SizeTotalFormattedRole,
        SizeDownloadedFormattedRole,
        SpeedDownFormattedRole,
        SpeedUpFormattedRole,
        EtaRole,
        EtaFormattedRole,
        PeersRole,
        SeedersRole,
        TypeRole,
        DestinationRole,
        UriRole,
        FilesRole,
        TrackersRole,
        RawDetailRole,
    };

    explicit DownloadModel(SynologyClient *client, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString formatSize(double bytes) const;
    Q_INVOKABLE QString formatSpeed(double bytesPerSec) const;
    Q_INVOKABLE QString formatEta(int seconds) const;

signals:
    void countChanged();

public slots:
    void onTasksReceived(const QJsonArray &tasks);

private:

    QList<DownloadTask> m_tasks;
    QTimer *m_pollTimer;
    SynologyClient *m_client;
};
