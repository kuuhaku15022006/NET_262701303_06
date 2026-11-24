#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariantList>

class Database
{
public:
    Database();
    ~Database();

    bool connectToMySQL(const QString &host,
                        int port,
                        const QString &databaseName,
                        const QString &username,
                        const QString &password);
    void close();

    bool initializeTables();
    bool execute(const QString &sql, const QVariantList &params = {});
    QSqlQuery query(const QString &sql, const QVariantList &params = {});
    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
    bool m_connected = false;
};
