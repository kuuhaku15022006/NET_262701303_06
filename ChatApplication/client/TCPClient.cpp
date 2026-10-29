#include "TCPClient.h"

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

    // Khởi tạo lắng nghe dữ liệu từ server
    receiveLoop();
}

TCPClient::~TCPClient()
{
    disconnectFromServer();
}

// Conect to server
void TCPClient::connectToServer(const QString &host, quint16 port)
{
    // Đang có kết nối cũ thì cúp trước tránh 2 kết nối trùng nhau
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();

    m_buffer.clear(); // xóa dữ liệu lần trước 
    m_socket->connectToHost(host, port); // bắt đầu nối, không chờ kết quả
}

// Disconnect from server
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

// Khởi tạo vòng lặp lắng nghe dữ liệu từ socket
void TCPClient::receiveLoop()
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TCPClient::onReadyRead, Qt::UniqueConnection);
}

void TCPClient::onReadyRead()
{
}