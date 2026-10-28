
#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

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

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject &json);
};

#endif
