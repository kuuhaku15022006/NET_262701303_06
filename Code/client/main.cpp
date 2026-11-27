#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QDebug>
#include "TCPClient.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Tạo Mock Server lắng nghe ở cổng 5555 để test
    QTcpServer mockServer;
    QTcpSocket *serverSocket = nullptr;
    mockServer.listen(QHostAddress::LocalHost, 5555);

    TCPClient client;

    // 1. Nối các Signal của TCPClient ra màn hình Console để lấy "Kết quả thực tế"
    QObject::connect(&client, &TCPClient::connected, []() {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: connected()";
    });
    QObject::connect(&client, &TCPClient::disconnected, []() {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: disconnected()";
    });
    QObject::connect(&client, &TCPClient::connectionStateChanged, [&client](bool state) {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: connectionStateChanged(" << state
                 << ") | isConnected() ==" << client.isConnected();
    });
    QObject::connect(&client, &TCPClient::connectionError, [](const QString &err) {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: connectionError(" << err << ")";
    });
    QObject::connect(&client, &TCPClient::responseReceived, [](const QJsonObject &res) {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: responseReceived:"
                 << QJsonDocument(res).toJson(QJsonDocument::Compact);
    });
    QObject::connect(&client, &TCPClient::eventReceived, [](const QJsonObject &ev) {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: eventReceived:"
                 << QJsonDocument(ev).toJson(QJsonDocument::Compact);
    });
    QObject::connect(&client, &TCPClient::messageReceived, [](const QJsonObject &msg) {
        qDebug() << "  -> [KẾT QUẢ] Phát signal: messageReceived:"
                 << QJsonDocument(msg).toJson(QJsonDocument::Compact);
    });

    // 2. Kịch bản của Mock Server khi TCPClient kết nối thành công
    QObject::connect(&mockServer, &QTcpServer::newConnection, [&]() {
        serverSocket = mockServer.nextPendingConnection();

        // Xem dữ liệu từ hàm login() và registerUser() gửi lên (TC-NET-07)
        QObject::connect(serverSocket, &QTcpSocket::readyRead, [&]() {
            while (serverSocket->canReadLine()) {
                qDebug() << "  -> [KẾT QUẢ SERVER NHẬN]:" << serverSocket->readLine().trimmed();
            }
        });

        // Test TC-NET-03: Gửi 1 JSON bị cắt làm 2 mảnh cách nhau 200ms
        QTimer::singleShot(200, [&]() {
            qDebug() << "\n=== [TC-NET-03] Server gửi mảnh 1 (chưa có \\n) ===";
            serverSocket->write("{\"type\":\"response\",\"req");
            serverSocket->flush();
        });
        QTimer::singleShot(400, [&]() {
            qDebug() << "=== [TC-NET-03] Server gửi mảnh 2 (có \\n) ===";
            serverSocket->write("uest_id\":1,\"ok\":true}\n");
            serverSocket->flush();
        });

        // Test TC-NET-04: Gửi 3 JSON dính liền nhau trong 1 lần write
        QTimer::singleShot(600, [&]() {
            qDebug() << "\n=== [TC-NET-04] Server gửi 3 JSON liền nhau ===";
            serverSocket->write(
                "{\"type\":\"response\",\"request_id\":1,\"ok\":true}\n"
                "{\"type\":\"response\",\"request_id\":2,\"ok\":true}\n"
                "{\"type\":\"response\",\"request_id\":3,\"ok\":true}\n"
            );
            serverSocket->flush();
        });

        // Test TC-NET-05: Phân biệt response và event message_received
        QTimer::singleShot(800, [&]() {
            qDebug() << "\n=== [TC-NET-05] Server gửi 1 Response và 1 Event message_received ===";
            serverSocket->write("{\"type\":\"response\",\"request_id\":10,\"ok\":true}\n");
            serverSocket->write("{\"type\":\"event\",\"event\":\"message_received\",\"payload\":{\"id\":1,\"content\":\"Xin chao\"}}\n");
            serverSocket->flush();
        });

        // Test TC-NET-06: Server gửi JSON lỗi cú pháp '{abc'
        QTimer::singleShot(1000, [&]() {
            qDebug() << "\n=== [TC-NET-06] Server gửi JSON lỗi '{abc\\n' kèm 1 JSON đúng ===";
            serverSocket->write("{abc\n");
            serverSocket->write("{\"type\":\"response\",\"request_id\":99,\"ok\":true}\n");
            serverSocket->flush();
        });

        // Test TC-NET-07 (Phần login & registerUser)
        QTimer::singleShot(1200, [&]() {
            qDebug() << "\n=== [TC-NET-07] Gọi thử client.login() và client.registerUser() ===";
            client.login("user01", "Pass@123");
            client.registerUser("user02", "Pass@123", "Thai Nguyen");
        });

        // Ngắt kết nối để hoàn tất TC-NET-01
        QTimer::singleShot(1500, [&]() {
            qDebug() << "\n=== [TC-NET-01] Ngắt kết nối khỏi Server ===";
            client.disconnectFromServer();
            app.quit();
        });
    });

    // Bắt đầu chạy từ TC-NET-02 (Kết nối cổng 5999 chưa mở)
    qDebug() << "=== [TC-NET-02] Kết nối vào port 5999 (Server chưa chạy) ===";
    client.connectToServer("127.0.0.1", 5999);

    // Sau đó chạy TC-NET-01 (Kết nối cổng 5555 đang mở)
    QTimer::singleShot(300, [&]() {
        qDebug() << "\n=== [TC-NET-01] Kết nối vào port 5555 (Mock Server đang chạy) ===";
        client.connectToServer("127.0.0.1", 5555);
    });

    return app.exec();
}