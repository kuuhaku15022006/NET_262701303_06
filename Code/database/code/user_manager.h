#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "database.h"

#include <QString>
#include <QVariantMap>
#include <QList>

class UserManager
{
public:
    explicit UserManager(Database &database);

    // Chuc nang 1: Dang ky tai khoan
    bool registerUser(
        const QString &username,
        const QString &password,
        const QString &displayName,
        QString *errorText = nullptr
    );

    // Chuc nang 2: Dang nhap tai khoan
    QVariantMap authenticate(
        const QString &username,
        const QString &password,
        QString *errorText = nullptr
    );

    // Chuc nang 3: Tim user
    bool findUsers(
        const QString &keyword,
        QList<QVariantMap> *users,
        QString *errorText = nullptr
    );

private:
    static QString createSalt();

    static QString hashPassword(
        const QString &password,
        const QString &salt
    );

    Database &m_database;
};

#endif