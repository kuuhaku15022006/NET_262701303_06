
#include "ClientHandler.h"

ClientHandler::ClientHandler(QTcpSocket* socket,
    QObject* parent)
    : QObject(parent), socket(socket)
{
    connect(socket, &QTcpSocket::readyRead,
        this, &ClientHandler::readData);

    connect(socket, &QTcpSocket::disconnected,
        this, &ClientHandler::handleDisconnect);
}

QTcpSocket* ClientHandler::getSocket() const
{
    return socket;
}

void ClientHandler::readData()
{
    buffer.append(socket->readAll());

    while (buffer.contains('\n')) {

        int position = buffer.indexOf('\n');

        QByteArray message = buffer.left(position).trimmed();

        buffer.remove(0, position + 1);

        if (!message.isEmpty()) {

            emit messageReceived(this, message);
        }
    }
}

void ClientHandler::handleDisconnect()
{
    emit clientDisconnected(this);
}
