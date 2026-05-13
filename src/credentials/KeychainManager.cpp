#include "KeychainManager.h"

#if defined(Q_OS_MACOS)
#include <Security/Security.h>
#elif defined(Q_OS_WIN)
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <wincred.h>
#include <windows.h>
#else
#include <QCoreApplication>
#include <QProcess>
#endif

void KeychainManager::save(const QString &service, const QString &key, const QString &secret)
{
    if (secret.isEmpty())
        return;

#if defined(Q_OS_MACOS)
    SecKeychainAddGenericPassword(nullptr,
                                  service.toUtf8().size(),
                                  service.toUtf8().constData(),
                                  key.toUtf8().size(),
                                  key.toUtf8().constData(),
                                  secret.toUtf8().size(),
                                  secret.toUtf8().constData(),
                                  nullptr);
#elif defined(Q_OS_WIN)
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    QString target = service + ":" + key;
    QByteArray targetUtf16 = QByteArray(reinterpret_cast<const char *>(target.utf16()), target.size() * 2 + 2);
    cred.TargetName = reinterpret_cast<LPWSTR>(targetUtf16.data());
    QByteArray secretUtf8 = secret.toUtf8();
    cred.CredentialBlobSize = secretUtf8.size();
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(secretUtf8.data());
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    CredWriteW(&cred, 0);
#else
    // Linux: use secret-tool (libsecret CLI) if available
    QProcess proc;
    proc.start("secret-tool", {"store", "--label=" + service, "service", service, "account", key});
    proc.waitForStarted();
    proc.write(secret.toUtf8());
    proc.closeWriteChannel();
    proc.waitForFinished(5000);
#endif
}

QString KeychainManager::load(const QString &service, const QString &key)
{
#if defined(Q_OS_MACOS)
    void *data = nullptr;
    UInt32 len = 0;
    if (SecKeychainFindGenericPassword(nullptr,
                                       service.toUtf8().size(),
                                       service.toUtf8().constData(),
                                       key.toUtf8().size(),
                                       key.toUtf8().constData(),
                                       &len,
                                       &data,
                                       nullptr)
        == errSecSuccess) {
        QByteArray result(static_cast<const char *>(data), len);
        SecKeychainItemFreeContent(nullptr, data);
        return QString::fromUtf8(result);
    }
#elif defined(Q_OS_WIN)
    CREDENTIALW *cred = nullptr;
    QString target = service + ":" + key;
    QByteArray targetUtf16 = QByteArray(reinterpret_cast<const char *>(target.utf16()), target.size() * 2 + 2);
    if (CredReadW(reinterpret_cast<LPCWSTR>(targetUtf16.data()), CRED_TYPE_GENERIC, 0, &cred)) {
        QByteArray result(reinterpret_cast<const char *>(cred->CredentialBlob), cred->CredentialBlobSize);
        CredFree(cred);
        return QString::fromUtf8(result);
    }
#else
    QProcess proc;
    proc.start("secret-tool", {"lookup", "service", service, "account", key});
    proc.waitForFinished(5000);
    if (proc.exitCode() == 0)
        return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
#endif
    return {};
}

void KeychainManager::remove(const QString &service, const QString &key)
{
#if defined(Q_OS_MACOS)
    SecKeychainItemRef item;
    if (SecKeychainFindGenericPassword(nullptr,
                                       service.toUtf8().size(),
                                       service.toUtf8().constData(),
                                       key.toUtf8().size(),
                                       key.toUtf8().constData(),
                                       nullptr,
                                       nullptr,
                                       &item)
        == errSecSuccess) {
        SecKeychainItemDelete(item);
        CFRelease(item);
    }
#elif defined(Q_OS_WIN)
    QString target = service + ":" + key;
    QByteArray targetUtf16 = QByteArray(reinterpret_cast<const char *>(target.utf16()), target.size() * 2 + 2);
    CredDeleteW(reinterpret_cast<LPCWSTR>(targetUtf16.data()), CRED_TYPE_GENERIC, 0);
#else
    QProcess proc;
    proc.start("secret-tool", {"clear", "service", service, "account", key});
    proc.waitForFinished(5000);
#endif
}
