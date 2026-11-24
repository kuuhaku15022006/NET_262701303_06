#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QByteArray>
#include <QString>

class TCPClient : public QObject
{
    Q_OBJECT
public:
    explicit TCPClient(QObject *parent = nullptr);
    ~TCPClient() override;

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    int sendRequest(const QJsonObject &request);

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName);
    void getConversations();
    void getHistory(int conversationId, int limit = 50, int offset = 0);
    void sendMessage(int conversationId, const QString &content, int replyTo = -1);
    void forwardMessage(int messageId, int targetConversationId);

signals:
    void connected();
    void disconnected();
    void connectionStateChanged(bool isConnected);
    void connectionError(const QString &errorMessage);

    void responseReceived(const QJsonObject &response);
    void eventReceived(const QJsonObject &eventData);
    void messageReceived(const QJsonObject &messagePayload);

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    void processLine(const QByteArray &line);

    QTcpSocket *m_socket = nullptr;
    QTimer *m_connectTimer = nullptr; // Đếm thời gian chờ kết nối (Checkpoint 4.5)
    QTimer *m_requestTimer = nullptr; // Đếm thời gian chờ server phản hồi (Checkpoint 4.5)
    QByteArray m_buffer;
    int m_nextRequestId = 1;
};

#endif