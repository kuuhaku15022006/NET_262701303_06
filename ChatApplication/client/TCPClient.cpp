#include "TCPClient.h"
#include <QJsonDocument>

// Hàm khởi tạo: tạo socket và nối các tín hiệu socket ra ngoài
TCPClient::TCPClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    // Socket kết nối thành công -> báo cho GUI
    connect(m_socket, &QTcpSocket::connected,
            this, &TCPClient::connected);

    // Socket bị ngắt kết nối -> báo cho GUI    
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TCPClient::disconnected);

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

// Gửi request dạng JSON: bắt buộc có type, request_id, payload (Mục 3.2 & Checkpoint 4.2)
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

void TCPClient::processLine(const QByteArray &)
{
}