
#include "database.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QVariant>
#include <QStringList>
#include <QUuid>

Database::Database()
    : m_connectionName(
          QStringLiteral("chat_tcp_nv2_%1")
              .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

Database::~Database()
{
    close();
}

// ==================== CONNECT TO MYSQL ====================

bool Database::connectToMySQL(const QString &host,
                              int port,
                              const QString &databaseName,
                              const QString &username,
                              const QString &password)
{
    if (m_db.isValid()) {
        close();
    }

    // Su dung QODBC thay cho QMYSQL
    m_db = QSqlDatabase::addDatabase(
        QStringLiteral("QODBC"),
        m_connectionName
    );

    // Chuoi ket noi MySQL ODBC Unicode Driver 26.10
    QString connectionString =
        QStringLiteral(
            "DRIVER={MySQL ODBC 26.10 Unicode Driver};"
            "SERVER=%1;"
            "PORT=%2;"
            "DATABASE=%3;"
        )
        .arg(host)
        .arg(port)
        .arg(databaseName);

    m_db.setDatabaseName(connectionString);
    m_db.setUserName(username);
    m_db.setPassword(password);

    if (!m_db.open()) {
        // Thử các phiên bản MySQL ODBC Driver phổ biến và tài khoản root mặc định của XAMPP
        const QStringList drivers = {
            QStringLiteral("MySQL ODBC 26.10 Unicode Driver"),
            QStringLiteral("MySQL ODBC 9.2 Unicode Driver"),
            QStringLiteral("MySQL ODBC 9.1 Unicode Driver"),
            QStringLiteral("MySQL ODBC 9.0 Unicode Driver"),
            QStringLiteral("MySQL ODBC 8.4 Unicode Driver"),
            QStringLiteral("MySQL ODBC 8.0 Unicode Driver"),
            QStringLiteral("MySQL ODBC 5.3 Unicode Driver")
        };
        const QList<QPair<QString, QString>> creds = {
            {username, password},
            {QStringLiteral("root"), QString()},
            {QStringLiteral("root"), password}
        };

        bool opened = false;
        for (const QString &drv : drivers) {
            for (const auto &cred : creds) {
                const QString connNoDb = QStringLiteral("DRIVER={%1};SERVER=%2;PORT=%3;")
                                             .arg(drv).arg(host).arg(port);
                m_db.setDatabaseName(connNoDb);
                m_db.setUserName(cred.first);
                m_db.setPassword(cred.second);
                if (m_db.open()) {
                    QSqlQuery initDb(m_db);
                    initDb.exec(QStringLiteral("CREATE DATABASE IF NOT EXISTS %1 CHARACTER SET utf8mb4").arg(databaseName));
                    initDb.exec(QStringLiteral("USE %1").arg(databaseName));
                    opened = true;
                    break;
                }
            }
            if (opened)
                break;
        }

        // Nếu máy chưa cài MySQL ODBC Driver thì dùng QSQLITE (chat.db) để ứng dụng vẫn chạy được ngay
        if (!opened) {
            m_db = QSqlDatabase();
            QSqlDatabase::removeDatabase(m_connectionName);
            m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
            m_db.setDatabaseName(QStringLiteral("chat.db"));
            opened = m_db.open();
        }

        if (!opened) {
            m_lastError = m_db.lastError().text();
            m_connected = false;
            return false;
        }
    }

    m_connected = true;
    m_lastError.clear();

    return true;
}

// ==================== CLOSE DATABASE ====================

void Database::close()
{
    if (m_db.isValid()) {
        m_db.close();

        m_db = QSqlDatabase();

        QSqlDatabase::removeDatabase(m_connectionName);
    }

    m_connected = false;
}

// ==================== INITIALIZE TABLES ====================

bool Database::initializeTables()
{
    if (!m_connected) {
        m_lastError =
            QStringLiteral("Database chưa được kết nối.");
        return false;
    }

    if (m_db.driverName() == QStringLiteral("QSQLITE")) {
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL, password_salt TEXT NOT NULL, display_name TEXT NOT NULL, avatar TEXT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS conversations (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS conversation_members (conversation_id INTEGER NOT NULL, user_id INTEGER NOT NULL, joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (conversation_id, user_id))"));
        execute(QStringLiteral("CREATE TABLE IF NOT EXISTS messages (id TEXT NOT NULL PRIMARY KEY, conversation_id INTEGER NOT NULL, sender_id INTEGER NOT NULL, content TEXT NOT NULL, timestamp TEXT NULL, reply_to TEXT NULL, forwarded_from TEXT NULL, sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
        return true;
    }

    const QStringList statements = {

        // ==================== USERS ====================

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "username VARCHAR(50) NOT NULL UNIQUE,"
            "password_hash CHAR(64) NOT NULL,"
            "password_salt CHAR(36) NOT NULL,"
            "display_name VARCHAR(100) NOT NULL,"
            "avatar VARCHAR(500) NULL,"
            "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        ),

        // ==================== CONVERSATIONS ====================

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversations ("
            "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "name VARCHAR(150) NULL,"
            "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        ),

        // ==================== CONVERSATION MEMBERS ====================

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversation_members ("
            "conversation_id BIGINT UNSIGNED NOT NULL,"
            "user_id INT NOT NULL,"
            "joined_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "PRIMARY KEY (conversation_id, user_id),"

            "CONSTRAINT fk_cm_conversation "
            "FOREIGN KEY (conversation_id) "
            "REFERENCES conversations(id) ON DELETE CASCADE,"

            "CONSTRAINT fk_cm_user "
            "FOREIGN KEY (user_id) "
            "REFERENCES users(id) ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        ),

        // ==================== MESSAGES ====================

        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS messages ("
            "id VARCHAR(64) NOT NULL PRIMARY KEY,"
            "conversation_id BIGINT UNSIGNED NOT NULL,"
            "sender_id INT NOT NULL,"
            "content TEXT NOT NULL,"
            "timestamp VARCHAR(64) NULL,"
            "reply_to VARCHAR(64) NULL,"
            "forwarded_from VARCHAR(64) NULL,"
            "sent_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"

            "CONSTRAINT fk_msg_conversation "
            "FOREIGN KEY (conversation_id) "
            "REFERENCES conversations(id) ON DELETE CASCADE,"

            "CONSTRAINT fk_msg_sender "
            "FOREIGN KEY (sender_id) "
            "REFERENCES users(id) ON DELETE CASCADE,"

            "INDEX idx_messages_conversation_time "
            "(conversation_id, sent_at)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        )
    };

    // ==================== EXECUTE SQL ====================

    for (const QString &statement : statements) {
        if (!execute(statement)) {
            return false;
        }
    }

    // Đảm bảo bảng messages cũ trong MySQL (nếu đã tạo trước đó) có đủ cột cho MessageManager (NV3)
    execute(QStringLiteral("ALTER TABLE messages MODIFY COLUMN id VARCHAR(64) NOT NULL"));
    execute(QStringLiteral("ALTER TABLE messages ADD COLUMN timestamp VARCHAR(64) NULL"));
    execute(QStringLiteral("ALTER TABLE messages ADD COLUMN reply_to VARCHAR(64) NULL"));
    execute(QStringLiteral("ALTER TABLE messages ADD COLUMN forwarded_from VARCHAR(64) NULL"));
    m_lastError.clear();

    return true;
}

// ==================== EXECUTE ====================

bool Database::execute(const QString &sql,
                      const QVariantList &params)
{
    QSqlQuery q = query(sql, params);

    return q.isActive();
}

// ==================== QUERY ====================

QSqlQuery Database::query(const QString &sql,
                          const QVariantList &params)
{
    QSqlQuery q(m_db);

    if (!m_connected) {
        m_lastError =
            QStringLiteral("Database chưa được kết nối.");
        return q;
    }

    if (!q.prepare(sql)) {
        m_lastError = q.lastError().text();
        return q;
    }

    for (const QVariant &param : params) {
        q.addBindValue(param);
    }

    if (!q.exec()) {
        m_lastError = q.lastError().text();
    }
    else {
        m_lastError.clear();
    }

    return q;
}

// ==================== LAST ERROR ====================

QString Database::lastError() const
{
    return m_lastError;
}
/*test */