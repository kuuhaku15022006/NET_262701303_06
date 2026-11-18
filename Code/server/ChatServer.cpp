
#include "ChatServer.h"

ChatServer::ChatServer(QObject* parent)
    : QObject(parent)
{
    // tao server
    server = new QTcpServer(this);

    connectionManager = new ConnectionManager(this);

    connect(server, &QTcpServer::newConnection,
        this, &ChatServer::handleNewConnection);
}

ChatServer::~ChatServer()
{
    stopServer();
}

bool ChatServer::registerUser(
    const QString& userId, QTcpSocket* socket)
{
    return connectionManager->addConnection(userId, socket);
}

bool ChatServer::sendToUser(
    const QString& userId, const QByteArray& message)
{
    return connectionManager->sendToUser(userId, message);
}

bool ChatServer::startServer(const QHostAddress& host, int port)
{
    if (server->isListening())
        return true;

    if (port < 1 || port > 65535)
        return false;

    if (!server->listen(host, static_cast<quint16>(port)))
        return false;

    emit serverStarted(static_cast<quint16>(port));

    return true;
}

QString ChatServer::errorString() const
{
    return server->errorString();
}

void ChatServer::stopServer()
{
    if (!server->isListening() && clients.isEmpty())
        return;

    server->close();

    while (!clients.isEmpty()) {

        ClientHandler* client = clients.takeLast();

        QTcpSocket* socket = client->getSocket();

        if (socket) {
            connectionManager->removeSocket(socket);

            socket->abort();
        }

        client->deleteLater();
    }

    emit serverStopped();
}

bool ChatServer::isRunning() const
{
    return server->isListening();
}

int ChatServer::clientCount() const
{
    return clients.size();
}

void ChatServer::handleNewConnection()
{
    while (server->hasPendingConnections()) {

        QTcpSocket* socket = server->nextPendingConnection();

        ClientHandler* client = new ClientHandler(socket, this);

        clients.append(client);

        connect(client, &ClientHandler::messageReceived,
            this, [this](ClientHandler* handler,
                const QByteArray& message) {

                    emit clientMessageReceived(
                        handler->getSocket(), message);
            });

        connect(client, &ClientHandler::clientDisconnected,
            this, [this](ClientHandler* handler) {

                QTcpSocket* socket = handler->getSocket();

                connectionManager->removeSocket(socket);

                clients.removeOne(handler);

                emit clientDisconnected(socket);

                handler->deleteLater();
            });

        emit clientConnected(socket);
    }
}
