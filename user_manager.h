#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "database.h"
#include <QString>
#include <QVariantMap>
class UserManager
{
public:
    explicit UserManager(Database &database);

    bool registerUser(
        const QString &username,
        const QString &password,
        const QString &displayName,
        QString *errorText = nullptr
    );
    QVariantMap authenticate(
    const QString &username,
    const QString &password,
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