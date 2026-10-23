#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QByteArray>
#include <QString>

class TCPClient : public QObject
{
    Q_OBJECT

public:
    explicit TCPClient(QObject *parent = nullptr);

    // Hàm kết nối
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // Hàm gửi request
    int sendRequest(const QJsonObject &request);

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName);
    void getConversations();
    void getHistory(int conversationId);
    void sendMessage(int conversationId, const QString &content, int replyTo = -1);
    void forwardMessage(int messageId, int targetConversationId);
    
    // Thông báo sự kiện cho các thành phần khác như kết nối thành công hay nhận tin nhắn
signals:
    void connected();
    void disconnected();
    void connectionError(const QString &message);
    void responseReceived(const QJsonObject &response);
    void eventReceived(const QJsonObject &event);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void processLine(const QByteArray &line);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    int m_nextRequestId = 1;
};