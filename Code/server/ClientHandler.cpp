#include "ClientHandler.h"
#include <QDebug>
#include <QMetaObject>
#include <QScopedValueRollback>

namespace {
    // Gioi han moi frame 1 MiB, doc tung khoi 64 KiB va toi da 64 frame moi luot.
    constexpr qsizetype maxFrameBytes = 1024 * 1024;
    constexpr qint64 readChunkBytes = 64 * 1024;
    constexpr int maxFramesPerTurn = 64;
}

// Khoi tao bo xu ly rieng cho mot socket va dang ky su kien doc/ngat ket noi.
ClientHandler::ClientHandler(QTcpSocket* socket, QObject* parent)
    : QObject(parent), socket(socket)
{
    Q_ASSERT(socket);
    // Gan socket lam con cua ClientHandler de Qt tu giai phong khi huy handler.
    socket->setParent(this);
    socket->setReadBufferSize(readChunkBytes);
    connect(socket, &QTcpSocket::readyRead, this, &ClientHandler::readData);
    connect(socket, &QTcpSocket::disconnected, this, &ClientHandler::handleDisconnect);
}

// Tra con tro socket; QPointer tu ve nullptr neu doi tuong da bi huy.
QTcpSocket* ClientHandler::getSocket() const { return socket.data(); }

// Dat lich doc tiep trong event loop, tranh goi long nhau hoac dat lich trung.
void ClientHandler::scheduleRead()
{
    if (!socket || disconnected || readScheduled) return;
    readScheduled = true;
    QMetaObject::invokeMethod(this, [this] {
        readScheduled = false;
        readData();
        }, Qt::QueuedConnection);
}

// Xoa buffer va ngat ket noi khi frame vuot gioi han cho phep.
void ClientHandler::rejectOversizedFrame()
{
    qWarning() << "Khung du lieu TCP qua lon; dang ngat ket noi client";
    buffer.clear();
    if (socket) socket->abort();
}

// Doc byte TCP, ghep vao buffer, tach frame theo newline va phat signal khi du JSON.
void ClientHandler::readData()
{
    if (!socket || disconnected || reading) return;
    // Tu khoi phuc co reading khi ham ket thuc, ke ca khi return som.
    QScopedValueRollback<bool> guard(reading, true);

    // Xu ly het frame da du trong buffer truoc khi doc them byte moi.
    if (!buffer.contains('\n'))
        buffer.append(socket->read(readChunkBytes));

    qsizetype consumed = 0;
    int frames = 0;
    while (frames < maxFramesPerTurn) {
        // Tim ky tu xuong dong de xac dinh ranh gioi frame.
        const qsizetype end = buffer.indexOf('\n', consumed);
        if (end < 0) {
            if (buffer.size() - consumed > maxFrameBytes) {
                rejectOversizedFrame();
                return;
            }
            break; // TCP co the chia mot JSON thanh nhieu lan doc.
        }
        if (end - consumed > maxFrameBytes) {
            rejectOversizedFrame();
            return;
        }
        // Lay mot frame hoan chinh, bo khoang trang thua o hai dau.
        const QByteArray message = buffer.mid(consumed, end - consumed).trimmed();
        consumed = end + 1;
        ++frames;
        if (!message.isEmpty()) emit messageReceived(this, message);
        // Callback hoac luc gui phan hoi co the dong socket ngay lap tuc.
        if (!socket || disconnected || socket->state() != QAbstractSocket::ConnectedState)
            return;
    }

    // Xoa cac byte da xu ly mot lan de giam chi phi dich chuyen buffer.
    if (consumed > 0) buffer.remove(0, consumed);
    if (buffer.size() > maxFrameBytes && !buffer.contains('\n')) {
        rejectOversizedFrame();
        return;
    }
    // Gioi han cong viec moi luot de client gui nhieu khong chiem event loop.
    if (buffer.contains('\n') || socket->bytesAvailable() > 0)
        scheduleRead();
}

// Danh dau da ngat ket noi, don buffer va thong bao cho ChatServer.
void ClientHandler::handleDisconnect()
{
    if (disconnected) return;
    disconnected = true;
    buffer.clear();
    emit clientDisconnected(this);
}
