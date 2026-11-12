
#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>     
#include <QTcpSocket> 
#include <QByteArray>  

class ClientHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket* socket,
        QObject* parent = nullptr);

    QTcpSocket* getSocket() const;

signals:
    void messageReceived(ClientHandler* client,
        const QByteArray& message);

    void clientDisconnected(ClientHandler* client);

private slots:
    void readData();

    void handleDisconnect();

private:
    QTcpSocket* socket;

    QByteArray buffer;
};

#endif // CLIENTHANDLER_H
