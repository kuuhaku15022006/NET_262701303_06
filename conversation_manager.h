#pragma once

#include "database.h"
#include <QVariantMap>

class ConversationManager
{
public:
    explicit ConversationManager(Database &database);

    QList<QVariantMap> getUserConversations(qint64 userId,
                                            QString *errorText = nullptr);
    QList<QVariantMap> getMembers(qint64 conversationId,
                                  QString *errorText = nullptr);

    // Utility for tests/admin workflows; can be restricted later to server-side use.
    qint64 createConversation(const QString &name,
                              const QList<qint64> &memberIds,
                              QString *errorText = nullptr);

private:
    Database &m_database;
};
