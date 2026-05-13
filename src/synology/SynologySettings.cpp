#include "SynologySettings.h"
#include "credentials/KeychainManager.h"

SynologySettings::SynologySettings(QObject *parent)
    : QObject(parent)
    , m_settings("DSDesk", "DSDesk")
{
}

QString SynologySettings::host() const
{
    return m_settings.value("connection/host", "").toString();
}
void SynologySettings::setHost(const QString &host)
{
    m_settings.setValue("connection/host", host);
    emit hostChanged();
}

quint16 SynologySettings::port() const
{
    return m_settings.value("connection/port", 5000).toUInt();
}
void SynologySettings::setPort(quint16 port)
{
    m_settings.setValue("connection/port", port);
    emit portChanged();
}

QString SynologySettings::username() const
{
    return m_settings.value("connection/username", "").toString();
}
void SynologySettings::setUsername(const QString &username)
{
    m_settings.setValue("connection/username", username);
    emit usernameChanged();
}

bool SynologySettings::useSsl() const
{
    return m_settings.value("connection/useSsl", false).toBool();
}
void SynologySettings::setUseSsl(bool useSsl)
{
    m_settings.setValue("connection/useSsl", useSsl);
    emit useSslChanged();
}

bool SynologySettings::trustCert() const
{
    return m_settings.value("connection/trustCert", false).toBool();
}
void SynologySettings::setTrustCert(bool trustCert)
{
    m_settings.setValue("connection/trustCert", trustCert);
    emit trustCertChanged();
}

QString SynologySettings::language() const
{
    return m_settings.value("app/language", "auto").toString();
}
void SynologySettings::setLanguage(const QString &lang)
{
    m_settings.setValue("app/language", lang);
    emit languageChanged();
}

void SynologySettings::saveCredentials(const QString &password)
{
    KeychainManager::save("DSDesk", m_settings.value("connection/username").toString(), password);
    m_settings.setValue("connection/hasCredentials", true);
}

QString SynologySettings::loadPassword()
{
    if (!m_settings.value("connection/hasCredentials", false).toBool())
        return {};
    return KeychainManager::load("DSDesk", m_settings.value("connection/username").toString());
}

void SynologySettings::clearCredentials()
{
    KeychainManager::remove("DSDesk", m_settings.value("connection/username").toString());
    m_settings.setValue("connection/hasCredentials", false);
}
