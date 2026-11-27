#include "conversation_manager.h"
#include "database.h"

#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDebug>

// Constructor
ConversationManager::ConversationManager(Database &db)
    : database(db)
{
}

// ==========================================
// 4. Danh sach hoi thoai theo User ID
// ==========================================
QVariantList ConversationManager::getConversationsByUserId(int userId)
{
    QVariantList result;

    QSqlQuery query = database.query(
        "SELECT c.id, c.name, c.created_at "
        "FROM conversations c "
        "JOIN conversation_members cm "
        "ON c.id = cm.conversation_id "
        "WHERE cm.user_id = ?",
        {userId}
    );

    if (!query.isActive())
    {
        qDebug() << "Loi truy van danh sach hoi thoai:"
                 << database.lastError();
        return result;
    }

    while (query.next())
    {
        QVariantMap conversation;

        conversation["id"] = query.value("id");
        conversation["name"] = query.value("name");
        conversation["created_at"] = query.value("created_at");

        result.append(conversation);
    }

    return result;
}

// ==========================================
// 5. Danh sach thanh vien theo Conversation ID
// ==========================================
QVariantList ConversationManager::getMembersByConversationId(
    int conversationId)
{
    QVariantList result;

    QSqlQuery query = database.query(
        "SELECT u.id, u.username, u.display_name, u.avatar "
        "FROM users u "
        "INNER JOIN conversation_members cm "
        "ON u.id = cm.user_id "
        "WHERE cm.conversation_id = ?",
        {conversationId}
    );

    if (!query.isActive())
    {
        qDebug() << "Loi truy van danh sach thanh vien:"
                 << database.lastError();
        return result;
    }

    while (query.next())
    {
        QVariantMap member;

        member["id"] = query.value("id");
        member["username"] = query.value("username");
        member["display_name"] = query.value("display_name");
        member["avatar"] = query.value("avatar");

        result.append(member);
    }

    return result;
}

void ConversationManager::ensureDefaultConversationsForUser(int userId)
{
    if (userId <= 0)
        return;

    // Đảm bảo có ít nhất 2 phòng chat chung để user có thể nhắn tin và forward sang hội thoại khác
    const QStringList defaultRooms = {
        QStringLiteral("Phòng Chat Chung"),
        QStringLiteral("Nhóm Học Tập")
    };

    for (const QString &roomName : defaultRooms) {
        QSqlQuery checkRoom = database.query(
            QStringLiteral("SELECT id FROM conversations WHERE name = ?"),
            {roomName}
        );

        int convId = 0;
        if (checkRoom.isActive() && checkRoom.next()) {
            convId = checkRoom.value(0).toInt();
        } else {
            database.execute(
                QStringLiteral("INSERT INTO conversations (name) VALUES (?)"),
                {roomName}
            );
            QSqlQuery recheck = database.query(
                QStringLiteral("SELECT id FROM conversations WHERE name = ?"),
                {roomName}
            );
            if (recheck.isActive() && recheck.next()) {
                convId = recheck.value(0).toInt();
            }
        }

        if (convId > 0) {
            QSqlQuery checkMember = database.query(
                QStringLiteral("SELECT 1 FROM conversation_members WHERE conversation_id = ? AND user_id = ?"),
                {convId, userId}
            );
            if (checkMember.isActive() && !checkMember.next()) {
                database.execute(
                    QStringLiteral("INSERT INTO conversation_members (conversation_id, user_id) VALUES (?, ?)"),
                    {convId, userId}
                );
            }
        }
    }
}

/*test */