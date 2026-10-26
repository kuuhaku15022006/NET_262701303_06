#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QString>

class TCPClient : public QObject
{
    Q_OBJECT

public:
    explicit TCPClient(QObject *parent = nullptr);
    ~TCPClient() override;

    // Hàm quản lý kết nối
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // Hàm gửi request và vòng lặp nhận dữ liệu
    void receiveLoop();
    int sendRequest(const QJsonObject &request);

    // Hàm xác thực người dùng
    void login(const QString &username, const QString &password);
    void registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName);

    // Hàm lấy danh sách hội thoại và lịch sử chat
    void getConversations();
    void getHistory(int conversationId, int limit = 50, int offset = 0);

    // Hàm gửi, phản hồi và chuyển tiếp tin nhắn
    void sendMessage(int conversationId, const QString &content, int replyTo = -1);
    void forwardMessage(int messageId, int targetConversationId);

signals:
    // Tín hiệu trạng thái kết nối
    void connected();
    void disconnected();
    void connectionError(const QString &message);

    // Tín hiệu nhận dữ liệu chung từ server
    void responseReceived(const QJsonObject &response);
    void eventReceived(const QJsonObject &event);

    // Tín hiệu chi tiết cho từng chức năng trên GUI
    void loginResult(bool success, const QString &message, const QJsonObject &userData);
    void registerResult(bool success, const QString &message);
    void conversationsReceived(const QJsonArray &conversations);
    void historyReceived(int conversationId, const QJsonArray &messages);
    void newMessageReceived(const QJsonObject &message);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void processLine(const QByteArray &line);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    int m_nextRequestId = 1;
};