#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "ConnectionManager.h"
#include "ClientHandler.h"
#include "../shared/Protocol.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QHostAddress>
#include <QJsonObject>

class ChatServer : public QObject
{
    Q_OBJECT
public:
    // Khoi tao Server.
    explicit ChatServer(QObject* parent = nullptr);
    // Huy Server.
    ~ChatServer() override;
    // Lay loi khoi dong gan nhat.
    QString errorString() const;
    // Dang ky anh xa user/socket sau xac thuc.
    bool registerUser(const QString& userId, QTcpSocket* socket);
    // Gui theo User ID.
    bool sendToUser(const QString& userId, const QByteArray& message);
    // Gui truc tiep qua socket.
    bool sendToSocket(QTcpSocket* socket, const QByteArray& message);
    // Tra cuu User ID tu socket.
    QString getUserId(QTcpSocket* socket) const;
    // Bat dau listen tren IP va port.
    bool startServer(const QHostAddress& host, int port);
    // Dung Server va don ket noi.
    void stopServer();
    // Kiem tra trang thai listen.
    bool isRunning() const;
    // Dem client TCP dang ket noi.
    int clientCount() const;

signals:
    // Phat khi listen thanh cong.
    void serverStarted(quint16 port);
    // Phat khi Server da dung.
    void serverStopped();
    // Phat khi co client moi.
    void clientConnected(QTcpSocket* socket);
    // Phat khi client da duoc xoa khoi danh sach.
    void clientDisconnected(QTcpSocket* socket);
    // Phat khi nhan frame JSON hop le.
    void clientMessageReceived(QTcpSocket* socket, const QByteArray& message);
    // Phat JSON object cho router xu ly.
    void clientRequestReceived(QTcpSocket* socket, const QJsonObject& request);

private slots:
    // Slot xu ly ket noi moi tu QTcpServer.
    void handleNewConnection();

private:
    // Kiem tra socket thuoc Server.
    bool hasSocket(QTcpSocket* socket) const;
    // Don mot client khoi danh sach.
    void removeClient(QTcpSocket* key, bool abortSocket);
    // Doi tuong lang nghe TCP.
    QTcpServer* server;
    // Danh sach handler theo socket.
    QHash<QTcpSocket*, ClientHandler*> clients;
    // Quan ly cac tai khoan da xac thuc.
    ConnectionManager* connectionManager;
    // Thong bao loi gan nhat.
    QString lastError;
    // Co danh dau qua trinh dung Server.
    bool stopping = false;
};
#endif
