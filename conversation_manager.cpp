#include "conversation_manager.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

ConversationManager::ConversationManager(Database &db)
    : database(db)
{
}

// 4. Danh sach hoi thoai theo User ID
QVariantList ConversationManager::getConversationsByUserId(int userId)
{
    QVariantList result;

    QSqlQuery query(database.getDatabase());

    query.prepare(
        "SELECT c.id, c.name, c.type, c.created_at "
        "FROM conversations c "
        "INNER JOIN conversation_members cm "
        "ON c.id = cm.conversation_id "
        "WHERE cm.user_id = :userId"
    );

    query.bindValue(":userId", userId);

    if (!query.exec()) {
        qDebug() << "Loi truy van danh sach hoi thoai:"
                 << query.lastError().text();
        return result;
    }

    while (query.next()) {
        QVariantMap conversation;

        conversation["id"] = query.value("id");
        conversation["name"] = query.value("name");
        conversation["type"] = query.value("type");
        conversation["created_at"] = query.value("created_at");

        result.append(conversation);
    }

    return result;
}

// 5. Danh sach thanh vien theo Conversation ID
QVariantList ConversationManager::getMembersByConversationId(
    int conversationId)
{
    QVariantList result;

    QSqlQuery query(database.getDatabase());

    query.prepare(
        "SELECT u.id, u.username, u.display_name, u.avatar "
        "FROM users u "
        "INNER JOIN conversation_members cm "
        "ON u.id = cm.user_id "
        "WHERE cm.conversation_id = :conversationId"
    );

    query.bindValue(":conversationId", conversationId);

    if (!query.exec()) {
        qDebug() << "Loi truy van danh sach thanh vien:"
                 << query.lastError().text();
        return result;
    }

    while (query.next()) {
        QVariantMap member;

        member["id"] = query.value("id");
        member["username"] = query.value("username");
        member["display_name"] = query.value("display_name");
        member["avatar"] = query.value("avatar");

        result.append(member);
    }

    return result;
}