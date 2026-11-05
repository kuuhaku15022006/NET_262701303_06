#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QByteArray>
#include <QString>

class TCPClient : public QObject
{
    Q_OBJECT

public:
    explicit TCPClient(QObject *parent = nullptr);
    ~TCPClient() override;

    // Hàm quản lý kết nối (Checkpoint 4.1)
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const; // kiểm tra client có kết nối hay ko

    // Hàm gửi request (Checkpoint 4.2)
    int sendRequest(const QJsonObject &request);

    // Các hàm API nghiệp vụ cho GUI (Checkpoint 4.4)
    void login(const QString &username, const QString &password);
    void registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName);
    void getConversations();
    void getHistory(int conversationId, int limit = 50, int offset = 0);
    void sendMessage(int conversationId, const QString &content, int replyTo = -1);
    void forwardMessage(int messageId, int targetConversationId);

signals:
    // Tín hiệu trạng thái kết nối (Checkpoint 4.1 & 4.3)
    void connected();
    void disconnected();
    void connectionStateChanged(bool isConnected);
    void connectionError(const QString &message);

    // Tín hiệu nhận response và event realtime từ server (Checkpoint 4.3 & 6.2)
    void responseReceived(const QJsonObject &response);
    void eventReceived(const QJsonObject &event);
    void messageReceived(const QJsonObject &message);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void processLine(const QByteArray &line);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    int m_nextRequestId = 1;
};