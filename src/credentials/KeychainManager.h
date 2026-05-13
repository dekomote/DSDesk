#pragma once

#include <QString>

class KeychainManager
{
public:
    static void save(const QString &service, const QString &key, const QString &secret);
    static QString load(const QString &service, const QString &key);
    static void remove(const QString &service, const QString &key);
};
