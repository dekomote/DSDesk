#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class SynologySettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(bool useSsl READ useSsl WRITE setUseSsl NOTIFY useSslChanged)
    Q_PROPERTY(bool trustCert READ trustCert WRITE setTrustCert NOTIFY trustCertChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit SynologySettings(QObject *parent = nullptr);

    QString host() const;
    void setHost(const QString &host);

    quint16 port() const;
    void setPort(quint16 port);

    QString username() const;
    void setUsername(const QString &username);

    bool useSsl() const;
    void setUseSsl(bool useSsl);

    bool trustCert() const;
    void setTrustCert(bool trustCert);

    QString language() const;
    void setLanguage(const QString &lang);

    Q_INVOKABLE void saveCredentials(const QString &password);
    Q_INVOKABLE QString loadPassword();
    Q_INVOKABLE void clearCredentials();

signals:
    void hostChanged();
    void portChanged();
    void usernameChanged();
    void useSslChanged();
    void trustCertChanged();
    void languageChanged();

private:
    QSettings m_settings;
};
