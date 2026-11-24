#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QHash>
#include <QString>
#include <QByteArray>

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    // Khoi tao bo quan ly ket noi.
    explicit ConnectionManager(QObject* parent = nullptr);
    // Them anh xa user/socket.
    bool addConnection(const QString& userId, QTcpSocket* socket);
    // Xoa anh xa theo User ID.
    void removeConnection(const QString& userId);
    // Xoa anh xa theo socket.
    void removeSocket(QTcpSocket* socket);
    // Gui du lieu theo User ID.
    bool sendToUser(const QString& userId, const QByteArray& message);
    // Gui du lieu qua socket.
    bool sendToSocket(QTcpSocket* socket, const QByteArray& message);
    // Tra cuu socket tu User ID.
    QTcpSocket* getSocket(const QString& userId) const;
    // Tra cuu User ID tu socket.
    QString getUserId(QTcpSocket* socket) const;
    // Dem tai khoan da dang ky ket noi.
    int connectionCount() const;

private:
    // Cap nhat dong thoi hai bang anh xa trong thread Qt cua Server.
    // Bang tra cuu User ID -> socket.
    QHash<QString, QTcpSocket*> connections;
    // Bang tra cuu socket -> User ID.
    QHash<QTcpSocket*, QString> userIds;
    // Xoa dong thoi hai bang anh xa.
    void forgetSocket(QTcpSocket* socket);
};
#endif
