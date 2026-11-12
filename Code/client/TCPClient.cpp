#include "TCPClient.h"
#include <QJsonDocument>
#include <QJsonParseError>

// Hàm khởi tạo: tạo socket và nối các tín hiệu socket ra ngoài
TCPClient::TCPClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    // Socket kết nối thành công -> báo cho GUI
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        emit connected();
        emit connectionStateChanged(true);
    });

    // Socket bị ngắt kết nối -> báo cho GUI    
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        emit disconnected();
        emit connectionStateChanged(false);
    });

    // Socket gặp lỗi -> chạy onSocketError        
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, &TCPClient::onSocketError);

    // Socket có dữ liệu mới -> chạy onReadyRead để đọc vào buffer
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TCPClient::onReadyRead);
}

TCPClient::~TCPClient()
{
    disconnectFromServer();
}

// Conect to server (Checkpoint 4.1)
void TCPClient::connectToServer(const QString &host, quint16 port)
{
    // Đang có kết nối cũ thì cúp trước tránh 2 kết nối trùng nhau
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();

    m_buffer.clear(); // xóa dữ liệu lần trước 
    m_socket->connectToHost(host, port); // bắt đầu nối, không chờ kết quả
}

// Disconnect from server (Checkpoint 4.1)
void TCPClient::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();

    m_buffer.clear();
}

// Kiểm tra trạng thái kết nối 
bool TCPClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

// Socket gặp lỗi -> báo cho GUI
void TCPClient::onSocketError(QAbstractSocket::SocketError)
{
    emit connectionError(m_socket->errorString());
}

// Gửi request dạng JSON: bắt buộc có type, request_id, payload (Checkpoint 4.2)
int TCPClient::sendRequest(const QJsonObject &request)
{
    if (!isConnected()) {
        emit connectionError(QStringLiteral("Chưa kết nối tới server"));
        return -1;
    }

    // Kiểm tra trường bắt buộc "type" theo quy ước protocol
    if (!request.contains(QStringLiteral("type")) ||
        request.value(QStringLiteral("type")).toString().trimmed().isEmpty()) {
        emit connectionError(QStringLiteral("Request thiếu trường type"));
        return -1;
    }

    QJsonObject packet = request;

    // Gắn request_id tự tăng để khớp với response từ server
    int requestId = packet.value(QStringLiteral("request_id")).toInt(0);
    if (requestId <= 0) {
        requestId = m_nextRequestId++;
        packet.insert(QStringLiteral("request_id"), requestId);
    }

    // Đảm bảo luôn có trường payload theo quy ước protocol
    if (!packet.contains(QStringLiteral("payload"))) {
        packet.insert(QStringLiteral("payload"), QJsonObject());
    }

    // Chuyển JSON thành chuỗi byte trên 1 dòng (Compact) và thêm '\n'
    QByteArray data = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    data.append('\n');

    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        emit connectionError(m_socket->errorString());
        return -1;
    }

    return requestId;
}

// Nhận dữ liệu từ server, đưa vào buffer và tách từng dòng JSON hoàn chỉnh (Checkpoint 4.2)
void TCPClient::onReadyRead()
{
    // Đọc toàn bộ byte đang có trong socket nối vào bộ đệm
    m_buffer.append(m_socket->readAll());

    // Tách từng dòng phân cách bởi ký tự '\n'
    while (true) {
        int newlineIndex = m_buffer.indexOf('\n');
        if (newlineIndex == -1)
            break; // Chưa đủ một dòng JSON hoàn chỉnh, chờ lần readyRead tiếp theo

        // Lấy ra một dòng hoàn chỉnh và xóa phần đó khỏi buffer
        QByteArray line = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

// Phân tích dòng JSON, phân biệt response và event để phát signal cho GUI (Checkpoint 4.3)
void TCPClient::processLine(const QByteArray &line)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

    // Bỏ qua nếu dữ liệu không phải JSON hợp lệ để tránh làm crash app
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("response")) {
        emit responseReceived(obj);
    } else if (type == QStringLiteral("event")) {
        emit eventReceived(obj);

        // Nếu là sự kiện nhận tin nhắn mới thì phát ngay signal messageReceived cho GUI
        QString eventName = obj.value(QStringLiteral("event")).toString();
        if (eventName == QStringLiteral("message_received")) {
            QJsonObject payload = obj.value(QStringLiteral("payload")).toObject();
            emit messageReceived(payload.isEmpty() ? obj : payload);
        }
    }
}

// Gửi yêu cầu đăng nhập (Checkpoint 4.4)
void TCPClient::login(const QString &username, const QString &password)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("username"), username);
    payload.insert(QStringLiteral("password"), password);

    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("login"));
    request.insert(QStringLiteral("payload"), payload);

    sendRequest(request);
}

// Gửi yêu cầu đăng ký tài khoản 
void TCPClient::registerUser(const QString &username,
                             const QString &password,
                             const QString &displayName)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("username"), username);
    payload.insert(QStringLiteral("password"), password);
    payload.insert(QStringLiteral("display_name"), displayName);

    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("register"));
    request.insert(QStringLiteral("payload"), payload);

    sendRequest(request);
}

// Lấy danh sách các cuộc hội thoại của user hiện tại (Checkpoint 4.4)
void TCPClient::getConversations()
{
    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("get_conversations"));
    request.insert(QStringLiteral("payload"), QJsonObject()); // không cần tham số phụ

    sendRequest(request);
}

// Lấy lịch sử tin nhắn của một cuộc hội thoại, có hỗ trợ phân trang limit/offset (Checkpoint 4.4)
void TCPClient::getHistory(int conversationId, int limit, int offset)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("conversation_id"), conversationId);
    payload.insert(QStringLiteral("limit"), limit);
    payload.insert(QStringLiteral("offset"), offset);

    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("get_history"));
    request.insert(QStringLiteral("payload"), payload);

    sendRequest(request);
}

// Gửi tin nhắn mới (hỗ trợ cả gửi tin thường và trả lời tin nhắn gốc qua replyTo) (Checkpoint 4.4)
void TCPClient::sendMessage(int conversationId, const QString &content, int replyTo)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("conversation_id"), conversationId);
    payload.insert(QStringLiteral("content"), content);

    // Nếu replyTo > 0 thì gửi ID tin nhắn gốc, ngược lại để null theo quy ước protocol
    if (replyTo > 0) {
        payload.insert(QStringLiteral("reply_to"), replyTo);
    } else {
        payload.insert(QStringLiteral("reply_to"), QJsonValue::Null);
    }

    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("send_message"));
    request.insert(QStringLiteral("payload"), payload);

    sendRequest(request);
}

// Chuyển tiếp (forward) một tin nhắn sang cuộc hội thoại khác (Checkpoint 4.4)
void TCPClient::forwardMessage(int messageId, int targetConversationId)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("message_id"), messageId);
    payload.insert(QStringLiteral("target_conversation_id"), targetConversationId);

    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("forward_message"));
    request.insert(QStringLiteral("payload"), payload);

    sendRequest(request);
}
