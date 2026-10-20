
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTcpServer server;

    QObject::connect(
        &server,
        &QTcpServer::newConnection,
        [&server]() {
            while (server.hasPendingConnections()) {
                QTcpSocket *socket = server.nextPendingConnection();

                qDebug() << "Client connected:"
                         << socket->peerAddress().toString();

                QObject::connect(
                    socket,
                    &QTcpSocket::readyRead,
                    [socket]() {
                        QByteArray data = socket->readAll();
                        qDebug() << "Received:" << data;

                        socket->write(data); // Echo test
                    }
                );
            }
        }
    );

    if (!server.listen(QHostAddress::AnyIPv4, 5000)) {
        qCritical() << "Server error:" << server.errorString();
        return 1;
    }

    qDebug() << "Server listening on port 5000";

    return app.exec();
}
