
#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(QObject* parent)
    : QObject(parent)
{
}

bool ConnectionManager::addConnection(
    const QString& userId, QTcpSocket* socket)
{
    if (userId.isEmpty() || socket == nullptr)
        return false;

    if (socket->state() != QAbstractSocket::ConnectedState)
        return false;

    if (connections.contains(userId))
        return false;

    connections.insert(userId, socket);

    return true;
}

void ConnectionManager::removeConnection(
    const QString& userId)
{
    connections.remove(userId);
}

void ConnectionManager::removeSocket(QTcpSocket* socket)
{
    for (auto it = connections.begin();
        it != connections.end(); )
    {
        if (it.value() == socket)

            it = connections.erase(it);
        else

            ++it;
    }
}

bool ConnectionManager::sendToUser(
    const QString& userId, const QByteArray& message)
{
    QTcpSocket* socket = connections.value(userId, nullptr);

    if (socket == nullptr ||
        socket->state() != QAbstractSocket::ConnectedState)
        return false;

    QByteArray data = message + '\n';

    return socket->write(data) != -1;
}

QTcpSocket* ConnectionManager::getSocket(
    const QString& userId) const
{
    return connections.value(userId, nullptr);
}

int ConnectionManager::connectionCount() const
{
    return connections.size();
}
