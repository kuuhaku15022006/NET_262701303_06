#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include "../shared/message.h"

#include <QSqlDatabase>
#include <QList>
#include <QString>

class MessageManager
{
public:
    explicit MessageManager(QSqlDatabase database);

    bool send_message(
        const QString& conversationId,
        const QString& senderId,
        const QString& content,
        QString* newMessageId = nullptr,
        QString* error = nullptr
    );

    QList<Message> get_history(
        const QString& conversationId,
        int limit,
        int offset,
        QString* error = nullptr
    );

    bool reply_message(
        const QString& conversationId,
        const QString& senderId,
        const QString& content,
        const QString& replyTo,
        QString* newMessageId = nullptr,
        QString* error = nullptr
    );

    bool forward_message(
        const QString& messageId,
        const QString& targetConversationId,
        const QString& senderId,
        QString* newMessageId = nullptr,
        QString* error = nullptr
    );

    bool get_message(
        const QString& messageId,
        Message& result,
        QString* error = nullptr
    );

private:
    QSqlDatabase db;

    bool save_message(
        Message& message,
        QString* error
    );
};

#endif
