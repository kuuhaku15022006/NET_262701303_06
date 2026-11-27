
#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariant>

class Message
{
public:
    QString id;
    QString conversationId;
    QString senderId;
    QString content;
    QDateTime timestamp;

    
    QString replyTo;
    QString forwardedFrom;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), id);
        obj.insert(QStringLiteral("conversation_id"), conversationId);
        obj.insert(QStringLiteral("sender_id"), senderId);
        obj.insert(QStringLiteral("content"), content);
        obj.insert(QStringLiteral("timestamp"),
                   timestamp.isValid()
                       ? timestamp.toUTC().toString(Qt::ISODate)
                       : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        obj.insert(QStringLiteral("reply_to"),
                   replyTo.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(replyTo));
        obj.insert(QStringLiteral("forwarded_from"),
                   forwardedFrom.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(forwardedFrom));
        return obj;
    }

    static Message fromJson(const QJsonObject &json)
    {
        auto readStr = [&json](const char *k1, const char *k2 = nullptr) -> QString {
            QJsonValue v = json.value(QLatin1String(k1));
            if ((v.isUndefined() || v.isNull()) && k2)
                v = json.value(QLatin1String(k2));
            if (v.isString()) return v.toString();
            if (v.isDouble()) return QString::number(v.toVariant().toLongLong());
            return QString();
        };

        Message msg;
        msg.id = readStr("id", "message_id");
        msg.conversationId = readStr("conversation_id", "conversationId");
        msg.senderId = readStr("sender_id", "senderId");
        msg.content = readStr("content", "message");
        const QString ts = readStr("timestamp", "sent_at");
        msg.timestamp = ts.isEmpty()
            ? QDateTime::currentDateTimeUtc()
            : QDateTime::fromString(ts, Qt::ISODate).toUTC();
        msg.replyTo = readStr("reply_to", "replyTo");
        msg.forwardedFrom = readStr("forwarded_from", "forwardedFrom");
        return msg;
    }
};

#endif
