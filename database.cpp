#include "database.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QUuid>

Database::Database()
    : m_connectionName(QStringLiteral("chat_tcp_nv2_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

Database::~Database()
{
    close();
}

bool Database::connectToMySQL(const QString &host,
                              int port,
                              const QString &databaseName,
                              const QString &username,
                              const QString &password)
{
    if (m_db.isValid())
        close();

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), m_connectionName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(databaseName);
    m_db.setUserName(username);
    m_db.setPassword(password);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        m_connected = false;
        return false;
    }

    m_connected = true;
    m_lastError.clear();
    return true;
}

void Database::close()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    m_connected = false;
}

bool Database::initializeTables()
{
    if (!m_connected) {
        m_lastError = QStringLiteral("Database chưa được kết nối.");
        return false;
    }

    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "username VARCHAR(50) NOT NULL UNIQUE,"
            "password_hash CHAR(64) NOT NULL,"
            "password_salt CHAR(36) NOT NULL,"
            "display_name VARCHAR(100) NOT NULL,"
            "avatar VARCHAR(500) NULL,"
            "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversations ("
            "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "name VARCHAR(150) NULL,"
            "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversation_members ("
            "conversation_id BIGINT UNSIGNED NOT NULL,"
            "user_id BIGINT UNSIGNED NOT NULL,"
            "joined_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "PRIMARY KEY (conversation_id, user_id),"
            "CONSTRAINT fk_cm_conversation FOREIGN KEY (conversation_id) "
            "REFERENCES conversations(id) ON DELETE CASCADE,"
            "CONSTRAINT fk_cm_user FOREIGN KEY (user_id) "
            "REFERENCES users(id) ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS messages ("
            "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "conversation_id BIGINT UNSIGNED NOT NULL,"
            "sender_id BIGINT UNSIGNED NOT NULL,"
            "content TEXT NOT NULL,"
            "sent_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "CONSTRAINT fk_msg_conversation FOREIGN KEY (conversation_id) "
            "REFERENCES conversations(id) ON DELETE CASCADE,"
            "CONSTRAINT fk_msg_sender FOREIGN KEY (sender_id) "
            "REFERENCES users(id) ON DELETE CASCADE,"
            "INDEX idx_messages_conversation_time (conversation_id, sent_at)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
    };

    for (const QString &statement : statements) {
        if (!execute(statement))
            return false;
    }
    return true;
}

bool Database::execute(const QString &sql, const QVariantList &params)
{
    QSqlQuery q = query(sql, params);
    return q.isActive();
}

QSqlQuery Database::query(const QString &sql, const QVariantList &params)
{
    QSqlQuery q(m_db);
    if (!m_connected) {
        m_lastError = QStringLiteral("Database chưa được kết nối.");
        return q;
    }

    if (!q.prepare(sql)) {
        m_lastError = q.lastError().text();
        return q;
    }

    for (const QVariant &param : params)
        q.addBindValue(param);

    if (!q.exec())
        m_lastError = q.lastError().text();
    else
        m_lastError.clear();

    return q;
}

QString Database::lastError() const
{
    return m_lastError;
}
