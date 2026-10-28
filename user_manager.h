#pragma once

#include "database.h"
#include <QVariantMap>

class UserManager
{
public:
    explicit UserManager(Database &database);

    // Returns true when created. On failure, errorText explains why.
    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName,
                      QString *errorText = nullptr);

    // Returns the user row as a map when credentials are valid; otherwise empty.
    QVariantMap authenticate(const QString &username,
                             const QString &password,
                             QString *errorText = nullptr);

    QList<QVariantMap> searchUsers(const QString &keywords,
                                   qint64 excludeUserId = -1,
                                   QString *errorText = nullptr);

private:
    static QString createSalt();
    static QString hashPassword(const QString &password, const QString &salt);

    Database &m_database;
};
