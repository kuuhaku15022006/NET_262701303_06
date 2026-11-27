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
    void reconnect(); // Gọi nối lại tới server cũ (Checkpoint 4.6)
    void setAutoReconnect(bool enabled, int intervalMs = 3000); // Bật/tắt tự nối lại (Checkpoint 4.6)
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
    QTimer *m_connectTimer = nullptr;   // Đếm thời gian chờ kết nối (Checkpoint 4.5)
    QTimer *m_requestTimer = nullptr;   // Đếm thời gian chờ server phản hồi (Checkpoint 4.5)
    QTimer *m_reconnectTimer = nullptr; // Hẹn giờ tự nối lại khi rớt mạng (Checkpoint 4.6)
    QByteArray m_buffer;
    QString m_lastHost;                 // Lưu lại host cũ để reconnect (Checkpoint 4.6)
    quint16 m_lastPort = 0;             // Lưu lại port cũ để reconnect (Checkpoint 4.6)
    bool m_autoReconnect = false;       // Cờ bật/tắt tự động reconnect
    bool m_manualDisconnect = false;    // Phân biệt mình tự ngắt hay do server tắt
    int m_nextRequestId = 1;
};

#endif
