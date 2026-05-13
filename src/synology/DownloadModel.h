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

signals:
    void countChanged();

public slots:
    void onTasksReceived(const QJsonArray &tasks);

private:
    static QString formatSize(double bytes);
    static QString formatSpeed(double bytesPerSec);
    static QString formatEta(int seconds);

    QList<DownloadTask> m_tasks;
    QTimer *m_pollTimer;
    SynologyClient *m_client;
};
