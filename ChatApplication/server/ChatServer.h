
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "ConnectionManager.h"

#include <QObject>      
#include <QTcpServer>   
#include <QTcpSocket>   
#include <QList>         
#include <QHostAddress>  

#include "ClientHandler.h"

class ChatServer : public QObject
{
    Q_OBJECT

public:

    explicit ChatServer(QObject* parent = nullptr);

    ~ChatServer();

    QString errorString() const;

    bool registerUser(const QString& userId, QTcpSocket* socket);

    bool sendToUser(const QString& userId,
        const QByteArray& message);

    bool startServer(const QHostAddress& host, int port);

    void stopServer();

    bool isRunning() const;

    int clientCount() const;

signals:
    void serverStarted(quint16 port);

    void serverStopped();

    void clientConnected(QTcpSocket* socket);

    void clientDisconnected(QTcpSocket* socket);

    void clientMessageReceived(QTcpSocket* socket,
        const QByteArray& message);

private slots:
    void handleNewConnection();

private:
    QTcpServer* server;

    QList<ClientHandler*> clients;

    ConnectionManager* connectionManager;
};

#endif // CHATSERVER_H
