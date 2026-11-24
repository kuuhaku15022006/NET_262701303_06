#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QPointer>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    // Khoi tao handler cua mot client.
    explicit ClientHandler(QTcpSocket* socket, QObject* parent = nullptr);
    // Lay socket dang duoc quan ly.
    QTcpSocket* getSocket() const;

signals:
    // Phat khi co frame hoan chinh.
    void messageReceived(ClientHandler* client, const QByteArray& message);
    // Phat khi socket ngat ket noi.
    void clientDisconnected(ClientHandler* client);

private slots:
    // Doc du lieu va tach frame.
    void readData();
    // Xu ly su kien ngat ket noi.
    void handleDisconnect();

private:
    // Dat lich doc tiep trong event loop.
    void scheduleRead();
    // Ngat client gui frame qua lon.
    void rejectOversizedFrame();
    // Con tro an toan tu ve null khi socket bi huy.
    QPointer<QTcpSocket> socket;
    // Luu byte chua xu ly het.
    QByteArray buffer;
    // Danh dau ket noi da dong.
    bool disconnected = false;
    // Chan goi readData long nhau.
    bool reading = false;
    // Tranh dat lich doc trung lap.
    bool readScheduled = false;
};
#endif
