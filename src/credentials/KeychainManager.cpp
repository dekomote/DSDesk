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
    QByteArray serviceUtf8 = service.toUtf8();
    QByteArray keyUtf8 = key.toUtf8();
    QByteArray secretUtf8 = secret.toUtf8();

    CFDictionaryRef attributes = nullptr;
    const void *keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount,
        kSecValueData,
        kSecAttrAccessible
    };
    const void *values[] = {
        kSecClassGenericPassword,
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(serviceUtf8.constData()), serviceUtf8.size(), kCFStringEncodingUTF8, false),
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(keyUtf8.constData()), keyUtf8.size(), kCFStringEncodingUTF8, false),
        CFDataCreate(nullptr, reinterpret_cast<const UInt8 *>(secretUtf8.constData()), secretUtf8.size()),
        kSecAttrAccessibleWhenUnlocked
    };
    attributes = CFDictionaryCreate(nullptr, keys, values, 5, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    // Delete existing item first
    SecItemDelete(attributes);

    // Add new item
    OSStatus status = SecItemAdd(attributes, nullptr);

    CFRelease(attributes);
    CFRelease(values[1]);
    CFRelease(values[2]);
    CFRelease(values[3]);
#elif defined(Q_OS_WIN)
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    QString target = service + ":" + key;
    QByteArray targetUtf16 = QByteArray(reinterpret_cast<const char *>(target.utf16()), target.size() * 2 + 2);
    cred.TargetName = reinterpret_cast<LPWSTR>(targetUtf16.data());
    QByteArray secretUtf8 = secret.toUtf8();
    cred.CredentialBlobSize = secretUtf8.size();
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(secretUtf8.data());
    cred.Persist = CRED_PERSIST_ENTERPRISE;
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
    QByteArray serviceUtf8 = service.toUtf8();
    QByteArray keyUtf8 = key.toUtf8();

    const void *keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount,
        kSecReturnData,
        kSecMatchLimit
    };
    const void *values[] = {
        kSecClassGenericPassword,
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(serviceUtf8.constData()), serviceUtf8.size(), kCFStringEncodingUTF8, false),
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(keyUtf8.constData()), keyUtf8.size(), kCFStringEncodingUTF8, false),
        kCFBooleanTrue,
        kSecMatchLimitOne
    };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 5, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFTypeRef result = nullptr;
    OSStatus status = SecItemCopyMatching(query, &result);

    CFRelease(query);
    CFRelease(values[1]);
    CFRelease(values[2]);

    if (status == errSecSuccess && result) {
        QByteArray data = QByteArray::fromCFData(static_cast<CFDataRef>(result));
        CFRelease(result);
        return QString::fromUtf8(data);
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
    QByteArray serviceUtf8 = service.toUtf8();
    QByteArray keyUtf8 = key.toUtf8();

    const void *keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount
    };
    const void *values[] = {
        kSecClassGenericPassword,
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(serviceUtf8.constData()), serviceUtf8.size(), kCFStringEncodingUTF8, false),
        CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(keyUtf8.constData()), keyUtf8.size(), kCFStringEncodingUTF8, false)
    };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    SecItemDelete(query);

    CFRelease(query);
    CFRelease(values[1]);
    CFRelease(values[2]);
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
