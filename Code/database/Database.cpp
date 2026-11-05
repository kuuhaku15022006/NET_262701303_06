
#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool Database::connect()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("chat.db");

    if (!db.open()) {
        qDebug() << "Database error:"
                 << db.lastError().text();
        return false;
    }

    qDebug() << "Connected to SQLite successfully";
    return true;
}

bool Database::initialize()
{
    QSqlQuery query;

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            display_name TEXT
        )
    )");

    if (query.lastError().isValid()) {
        qDebug() << "Create table error:"
                 << query.lastError().text();
        return false;
    }

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            conversation_id INTEGER NOT NULL,
            sender_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            reply_to INTEGER,
            forwarded_from INTEGER
        )
    )");

    if (query.lastError().isValid()) {
        qDebug() << "Create messages error:"
                 << query.lastError().text();
        return false;
    }

    return true;
}
