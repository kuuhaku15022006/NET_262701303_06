#include "ConnectionManager.h"

namespace {
    // Gioi han frame 1 MiB va tong du lieu dang cho gui 4 MiB.
    constexpr qint64 maxFrameBytes = 1024 * 1024;
    constexpr qint64 maxQueuedBytes = 4 * 1024 * 1024;
}

// Khoi tao doi tuong quan ly anh xa tai khoan va socket.
ConnectionManager::ConnectionManager(QObject* parent) : QObject(parent) {}

// Them anh xa hai chieu sau xac thuc; tu choi socket hoac User ID bi trung.
bool ConnectionManager::addConnection(const QString& userId, QTcpSocket* socket)
{
    if (userId.trimmed().isEmpty() || !socket ||
        socket->state() != QAbstractSocket::ConnectedState)
        return false;
    // Mot tai khoan chi co mot socket da xac thuc; tu choi phien dang nhap thu hai.
    if (connections.contains(userId) || userIds.contains(socket))
        return false;

    // Luu ca hai huong de tra cuu nhanh khi gui tin va xac dinh nguoi gui.
    connections.insert(userId, socket);
    userIds.insert(socket, userId);
    // Don hai bang anh xa neu socket bi huy truc tiep.
    connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
        removeSocket(socket);
        });
    connect(socket, &QObject::destroyed, this, [this, socket] {
        forgetSocket(socket);
        });
    return true;
}

// Xoa dong thoi socket va User ID khoi hai bang anh xa.
void ConnectionManager::forgetSocket(QTcpSocket* socket)
{
    // Tim User ID tu socket truoc khi xoa khoi hai bang.
    const auto it = userIds.find(socket);
    if (it == userIds.end()) return;
    const QString id = it.value();
    userIds.erase(it);
    connections.remove(id);
}

// Huy dang ky ket noi theo User ID (khong truc tiep dong socket).
void ConnectionManager::removeConnection(const QString& userId)
{
    removeSocket(connections.value(userId, nullptr));
}

// Huy anh xa cua socket va ngat cac signal rieng cua manager.
void ConnectionManager::removeSocket(QTcpSocket* socket)
{
    if (!socket || !userIds.contains(socket)) return;
    forgetSocket(socket);
    // Chi huy lien ket signal cua ConnectionManager, khong anh huong ClientHandler.
    QObject::disconnect(socket, nullptr, this, nullptr);
}

// Tim socket cua User ID va chuyen du lieu den ham gui chung.
bool ConnectionManager::sendToUser(const QString& userId, const QByteArray& message)
{
    return sendToSocket(getSocket(userId), message);
}

// Kiem tra trang thai socket, dong khung JSON va ghi du lieu vao bo dem gui.
bool ConnectionManager::sendToSocket(QTcpSocket* socket, const QByteArray& message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState || message.isEmpty())
        return false;

    // Dam bao du lieu gui co ky tu newline theo giao thuc cua nhom.
    QByteArray data = message;
    if (!data.endsWith('\n')) data.append('\n');
    // Moi lan gui chi gom mot JSON compact va mot ky tu xuong dong o cuoi.
    if (data.size() - 1 > maxFrameBytes || data.indexOf('\n') != data.size() - 1)
        return false;
    // Gioi han bo dem gui de client doc cham khong lam tang bo nho vo han.
    if (socket->bytesToWrite() > maxQueuedBytes - data.size()) {
        socket->abort();
        return false;
    }
    const qint64 written = socket->write(data);
    if (written != data.size()) {
        // Dong socket neu Qt khong nhan du frame, tranh de frame dang do.
        socket->abort();
        return false;
    }
    return true; // Qt da nhan byte de gui, chua xac nhan client da nhan tin.
}

// Tra socket theo User ID; nullptr neu nguoi dung khong co ket noi da dang ky.
QTcpSocket* ConnectionManager::getSocket(const QString& userId) const
{
    return connections.value(userId, nullptr);
}

// Tra User ID tu socket; chuoi rong neu chua co anh xa.
QString ConnectionManager::getUserId(QTcpSocket* socket) const
{
    return userIds.value(socket, QString());
}

// Dem so tai khoan dang co anh xa ket noi.
int ConnectionManager::connectionCount() const
{
    return static_cast<int>(connections.size());
}
