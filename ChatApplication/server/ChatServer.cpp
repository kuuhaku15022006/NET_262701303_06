#include "MessageManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <algorithm>

MessageManager::MessageManager(QSqlDatabase database)
    : db(database)
{
}

bool MessageManager::save_message(
    Message& message,
    QString* error)
{
    if (!db.isOpen()) {
        if (error) *error = "Database chưa mở";
        return false;
    }

    if (message.id.isEmpty()) {
        message.id = QUuid::createUuid()
            .toString(QUuid::WithoutBraces);
    }

    if (!message.timestamp.isValid()) {
        message.timestamp = QDateTime::currentDateTimeUtc();
    }

    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO messages "
        "(id, conversation_id, sender_id, content, "
        "timestamp, reply_to, forwarded_from) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    );

    query.addBindValue(message.id);
    query.addBindValue(message.conversationId);
    query.addBindValue(message.senderId);
    query.addBindValue(message.content);
    query.addBindValue(
        message.timestamp.toUTC().toString(Qt::ISODate)
    );
    query.addBindValue(
        message.replyTo.isEmpty()
        ? QVariant()
        : QVariant(message.replyTo)
    );
    query.addBindValue(
        message.forwardedFrom.isEmpty()
        ? QVariant()
        : QVariant(message.forwardedFrom)
    );

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

bool MessageManager::send_message(
    const QString& conversationId,
    const QString& senderId,
    const QString& content,
    QString* newMessageId,
    QString* error)
{
    if (conversationId.trimmed().isEmpty()
        || senderId.trimmed().isEmpty()
        || content.trimmed().isEmpty()) {
        if (error) *error = "Thong tin tin nhan khong hop le";
        return false;
    }

    Message message;
    message.conversationId = conversationId;
    message.senderId = senderId;
    message.content = content;
    message.timestamp = QDateTime::currentDateTimeUtc();

    if (!save_message(message, error)) {
        return false;
    }

    if (newMessageId) *newMessageId = message.id;
    return true;
}

bool MessageManager::get_message(
    const QString& messageId,
    Message& result,
    QString* error)
{
    QSqlQuery query(db);

    query.prepare(
        "SELECT id, conversation_id, sender_id, content, "
        "timestamp, reply_to, forwarded_from "
        "FROM messages WHERE id = ?"
    );
    query.addBindValue(messageId);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        if (error) *error = "Khong tim thay tin nhan";
        return false;
    }

    result.id = query.value(0).toString();
    result.conversationId = query.value(1).toString();
    result.senderId = query.value(2).toString();
    result.content = query.value(3).toString();
    result.timestamp = QDateTime::fromString(
        query.value(4).toString(), Qt::ISODate
    ).toUTC();
    result.replyTo = query.value(5).toString();
    result.forwardedFrom = query.value(6).toString();

    return true;
}

QList<Message> MessageManager::get_history(
    const QString& conversationId,
    int limit,
    int offset,
    QString* error)
{
    QList<Message> messages;

    if (limit <= 0 || offset < 0) {
        if (error) *error = "Limit hoac offset khong hop le";
        return messages;
    }

    QSqlQuery query(db);

    query.prepare(
        "SELECT id, conversation_id, sender_id, content, "
        "timestamp, reply_to, forwarded_from "
        "FROM messages WHERE conversation_id = ? "
        "ORDER BY timestamp DESC LIMIT ? OFFSET ?"
    );

    query.addBindValue(conversationId);
    query.addBindValue(limit);
    query.addBindValue(offset);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return messages;
    }

    while (query.next()) {
        Message message;

        message.id = query.value(0).toString();
        message.conversationId = query.value(1).toString();
        message.senderId = query.value(2).toString();
        message.content = query.value(3).toString();
        message.timestamp = QDateTime::fromString(
            query.value(4).toString(), Qt::ISODate
        ).toUTC();
        message.replyTo = query.value(5).toString();
        message.forwardedFrom = query.value(6).toString();

        messages.append(message);
    }

    // Truy vấn mới nhất trước; đảo lại để hiển thị
    // theo thứ tự thời gian tăng dần.
    std::reverse(messages.begin(), messages.end());

    return messages;
}

bool MessageManager::reply_message(
    const QString& conversationId,
    const QString& senderId,
    const QString& content,
    const QString& replyTo,
    QString* newMessageId,
    QString* error)
{
    Message original;

    if (!get_message(replyTo, original, error)) {
        return false;
    }

    if (original.conversationId != conversationId) {
        if (error) *error =
            "Tin nhan goc khong thuoc cuoc hoi thoai nay";
        return false;
    }

    if (content.trimmed().isEmpty()) {
        if (error) *error = "Noi dung tra loi dang trong";
        return false;
    }

    Message message;
    message.conversationId = conversationId;
    message.senderId = senderId;
    message.content = content;
    message.replyTo = replyTo;
    message.timestamp = QDateTime::currentDateTimeUtc();

    if (!save_message(message, error)) {
        return false;
    }

    if (newMessageId) *newMessageId = message.id;
    return true;
}

bool MessageManager::forward_message(
    const QString& messageId,
    const QString& targetConversationId,
    const QString& senderId,
    QString* newMessageId,
    QString* error)
{
    Message original;

    if (!get_message(messageId, original, error)) {
        return false;
    }

    Message forwarded;
    forwarded.conversationId = targetConversationId;
    forwarded.senderId = senderId;
    forwarded.content = original.content;
    forwarded.forwardedFrom = original.id;
    forwarded.timestamp = QDateTime::currentDateTimeUtc();

    if (!save_message(forwarded, error)) {
        return false;
    }

    if (newMessageId) *newMessageId = forwarded.id;
    return true;
}
