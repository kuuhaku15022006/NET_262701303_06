#ifndef CONVERSATION_MANAGER_H
#define CONVERSATION_MANAGER_H

#include <QVariantList>
#include <QVariantMap>

class Database;

class ConversationManager
{
public:
    // Constructor
    explicit ConversationManager(Database &db);

    // Lay danh sach hoi thoai theo User ID
    QVariantList getConversationsByUserId(int userId);

    // Lay danh sach thanh vien theo Conversation ID
    QVariantList getMembersByConversationId(int conversationId);

    // Khoi tao hoi thoai mac dinh cho user de test chat va forward
    void ensureDefaultConversationsForUser(int userId);

private:
    Database &database;
};

#endif // CONVERSATION_MANAGER_H
/*test */