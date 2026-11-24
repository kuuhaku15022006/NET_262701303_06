#include "ChatServer.h"
#include <QDebug>
#include <QJsonValue>
#include <QPointer>
#include <QMetaObject>

namespace {
    constexpr int maxActiveClients = 256;
}

// Khoi tao TCP Server, bo quan ly ket noi va dang ky su kien ket noi moi.
ChatServer::ChatServer(QObject* parent)
    : QObject(parent), server(new QTcpServer(this)),
    connectionManager(new ConnectionManager(this))
{
    // Gioi han hang doi ket noi cho duoc tiep nhan.
    server->setMaxPendingConnections(128);
    connect(server, &QTcpServer::newConnection, this, &ChatServer::handleNewConnection);
}

// Huy Server an toan; dong tat ca ket noi truoc khi huy doi tuong.
ChatServer::~ChatServer()
{
    // Chan phat signal ra ben ngoai trong khi huy doi tuong Server.
    blockSignals(true);
    stopServer();
}

// Kiem tra socket co nam trong danh sach client dang duoc quan ly hay khong.
bool ChatServer::hasSocket(QTcpSocket* socket) const
{
    return socket && clients.contains(socket);
}

// Gan User ID da xac thuc voi socket; tra false neu ket noi khong hop le hoac bi trung.
bool ChatServer::registerUser(const QString& userId, QTcpSocket* socket)
{
    return hasSocket(socket) && connectionManager->addConnection(userId, socket);
}

// Tim socket theo User ID va gui du lieu den dung nguoi dung.
bool ChatServer::sendToUser(const QString& userId, const QByteArray& message)
{
    return sendToSocket(connectionManager->getSocket(userId), message);
}

// Gui phan hoi truc tiep den socket thuoc Server, ke ca khi chua dang nhap.
bool ChatServer::sendToSocket(QTcpSocket* socket, const QByteArray& message)
{
    return hasSocket(socket) && connectionManager->sendToSocket(socket, message);
}

// Tim User ID da xac thuc tu socket; tra chuoi rong neu chua dang nhap.
QString ChatServer::getUserId(QTcpSocket* socket) const
{
    return hasSocket(socket) ? connectionManager->getUserId(socket) : QString();
}

// Kiem tra dia chi IP va cong, sau do bat dau lang nghe ket noi TCP.
bool ChatServer::startServer(const QHostAddress& host, int port)
{
    lastError.clear();
    if (stopping || host.isNull() || port < 1 || port > 65535) {
        lastError = QStringLiteral("Server dang dung hoac dia chi IP/cong khong hop le");
        return false;
    }
    if (server->isListening()) {
        if (server->serverAddress() == host && server->serverPort() == port)
            return true;
        lastError = QStringLiteral("Server dang lang nghe tren dia chi IP hoac cong khac");
        return false;
    }
    // Neu cong bi chiem hoac khong co quyen, lay loi do Qt cung cap.
    if (!server->listen(host, static_cast<quint16>(port))) {
        lastError = server->errorString();
        return false;
    }
    emit serverStarted(server->serverPort());
    return true;
}

// Tra thong bao loi gan nhat cua qua trinh khoi dong Server.
QString ChatServer::errorString() const { return lastError; }

// Xoa handler va anh xa user; dong socket neu duoc yeu cau, sau do phat signal.
void ChatServer::removeClient(QTcpSocket* key, bool abortSocket)
{
    // Xoa muc khoi danh sach truoc de callback lap lai khong don dep hai lan.
    ClientHandler* handler = clients.take(key);
    if (!handler) return;
    QPointer<QTcpSocket> socket(handler->getSocket());
    disconnect(handler, nullptr, this, nullptr);
    if (socket) {
        connectionManager->removeSocket(socket.data());
        if (abortSocket) socket->abort();
    }
    handler->deleteLater();
    if (socket) emit clientDisconnected(socket.data());
    else qWarning() << "Socket cua client bi huy dot ngot; da xoa khoi danh sach ket noi";
}

// Dung lang nghe, don cac ket noi dang cho va cac client dang hoat dong.
void ChatServer::stopServer()
{
    if (stopping || (!server->isListening() && clients.isEmpty() && !server->hasPendingConnections()))
        return;
    // Chan cac thao tac khoi dong/ngat ket noi lap trong khi don dep.
    stopping = true;
    server->close();
    while (server->hasPendingConnections()) {
        auto* socket = server->nextPendingConnection();
        if (socket) { socket->abort(); socket->deleteLater(); }
    }
    while (!clients.isEmpty())
        removeClient(clients.begin().key(), true);
    stopping = false;
    emit serverStopped();
}

// Kiem tra Server co dang lang nghe ket noi hay khong.
bool ChatServer::isRunning() const { return server->isListening(); }
// Dem tat ca ket noi TCP dang quan ly, bao gom ca client chua dang nhap.
int ChatServer::clientCount() const { return static_cast<int>(clients.size()); }

// Nhan socket moi, tao ClientHandler va noi cac signal gui nhan du lieu.
void ChatServer::handleNewConnection()
{
    while (server->hasPendingConnections()) {
        auto* socket = server->nextPendingConnection();
        if (!socket) continue;
        if (stopping || !server->isListening() ||
            socket->state() != QAbstractSocket::ConnectedState ||
            clients.size() >= maxActiveClients) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
        // Moi client co mot handler rieng, nhung khong tao thread rieng.
        auto* client = new ClientHandler(socket, this);
        clients.insert(socket, client);

        connect(client, &ClientHandler::messageReceived, this,
            [this](ClientHandler* handler, const QByteArray& message) {
                QPointer<QTcpSocket> socket(handler->getSocket());
                if (!socket || !hasSocket(socket.data())) return;
                // NV3 cung cap Protocol de chuyen byte thanh JSON object.
                QJsonObject request;
                if (!Protocol::decode(message, request)) {
                    // Khong ghi log JSON goc vi login/register co the chua mat khau.
                    qWarning() << "Nhan duoc JSON khong hop le";
                    const QJsonObject response{
                        { "type", "response" },{ "request_id", QJsonValue::Null },
                        { "success", false },{ "payload", QJsonObject{} },
                        { "error", "JSON khong hop le hoac khong phai JSON object" }
                    };
                    sendToSocket(socket.data(), Protocol::encode(response));
                    return;
                }
                // Phat signal de module ben ngoai nhan du lieu; sau do chuyen cho router.
                emit clientMessageReceived(socket.data(), message);
                if (socket && hasSocket(socket.data()))
                    emit clientRequestReceived(socket.data(), request);
            });
        connect(client, &ClientHandler::clientDisconnected, this,
            [this, socket](ClientHandler*) { removeClient(socket, false); });
        // Thong thuong ClientHandler quan ly vong doi socket. Don ca muc cu
        // neu mot thanh phan ben ngoai huy socket dot ngot.
        connect(socket, &QObject::destroyed, this,
            [this, socket] { removeClient(socket, false); });

        // Callback clientConnected co the dung Server, nen kiem tra lai con tro.
        QPointer<QTcpSocket> aliveSocket(socket);
        QPointer<ClientHandler> aliveHandler(client);
        emit clientConnected(socket);
        if (aliveSocket && aliveHandler && hasSocket(aliveSocket.data()) &&
            aliveSocket->bytesAvailable() > 0)
            QMetaObject::invokeMethod(aliveHandler.data(), "readData", Qt::QueuedConnection);
    }
}
