
#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <QHostAddress>
#include <QMetaObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

#include <iostream>
#include <thread>

#include "ChatServer.h"
#include "../database/code/database.h"
#include "../database/code/user_manager.h"
#include "../database/code/conversation_manager.h"
#include "../chatServer/MessageManager.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream input(stdin);
    QTextStream output(stdout);

    // Khởi tạo Database theo code của NV2
    Database database;
    QString dbPassword = qEnvironmentVariable("CHAT_DB_PASSWORD");
    if (dbPassword.isEmpty())
        dbPassword = QStringLiteral("ChatTcp_2026_New!");

    if (database.connectToMySQL("127.0.0.1", 3306, "chat_tcp", "chat_app", dbPassword)) {
        database.initializeTables();
    } else {
        qWarning() << "Ket noi MySQL that bai:" << database.lastError();
    }

    UserManager users(database);
    ConversationManager conversations(database);
    MessageManager messages(database.sqlDatabase());

    QHostAddress host;
    int port = 0;

    while (true) {
        QString hostInput;
        QString portInput;

        output << "Enter host (ex: 127.0.0.1): ";
        output.flush();
        hostInput = input.readLine().trimmed();
        if (hostInput.isEmpty())
            hostInput = QStringLiteral("127.0.0.1");

        if (!host.setAddress(hostInput)) {
            qCritical() << "Invalid host address! Try again.";
            continue;
        }

        output << "Enter port (1->65535): ";
        output.flush();
        portInput = input.readLine().trimmed();
        if (portInput.isEmpty())
            portInput = QStringLiteral("5555");

        bool portOk = false;
        int inputPort = portInput.toInt(&portOk);

        if (!portOk || inputPort < 1 || inputPort > 65535) {
            qCritical() << "Invalid port! Please enter 1-65535.";
            continue;
        }

        port = inputPort;
        break;
    }

    ChatServer server;
    server.setManagers(&users, &conversations, &messages);

    QObject::connect(&server, &ChatServer::serverStarted,
        [](quint16 port) {
            qDebug() << "Server started on port:" << port;
        });

    QObject::connect(&server, &ChatServer::serverStopped,
        []() {
            qDebug() << "Server stopped";
        });

    int clientNumber = 0;

    QObject::connect(&server, &ChatServer::clientConnected,
        [&server, &clientNumber](QTcpSocket* socket) {
            clientNumber++;
            qDebug() << "Client connected #" << clientNumber
                     << "from" << socket->peerAddress().toString();
        });

    QObject::connect(&server, &ChatServer::clientDisconnected,
        [](QTcpSocket* socket) {
            qDebug() << "Client disconnected:"
                << socket->peerAddress().toString();
        });

    QObject::connect(&server, &ChatServer::clientMessageReceived,
        [](QTcpSocket* socket, const QByteArray& message) {
            qDebug() << "Received from"
                << socket->peerAddress().toString()
                << ":" << message;
        });

    QObject::connect(&server, &ChatServer::clientRequestReceived,
        [](QTcpSocket* socket, const QJsonObject& request) {
            qDebug() << "Decoded request from"
                << socket->peerAddress().toString()
                << ":" << QJsonDocument(request).toJson(QJsonDocument::Compact);
        });

    if (!server.startServer(host, port)) {
        qCritical() << "Server error:" << server.errorString();

        output << "Press Enter to exit...";
        output.flush();
        input.readLine();

        return 1;
    }

    std::thread inputThread([&]() {
        std::string command;

        while (std::cin >> command) {
            QMetaObject::invokeMethod(&app, [&, command]() {

                if (command == "send1") {
                    bool sent = server.sendToUser("test1",
                        R"({"type":"event","message":"Hello Client 1"})");
                    qDebug() << "Send to test1:" << sent;
                }
                else if (command == "send2") {
                    bool sent = server.sendToUser("test2",
                        R"({"type":"event","message":"Hello Client 2"})");
                    qDebug() << "Send to test2:" << sent;
                }
                else if (command == "stop") {
                    server.stopServer();
                    qDebug() << "Type start to restart or exit to quit.";
                }
                else if (command == "start") {
                    if (server.startServer(host, port)) {
                        qDebug() << "Server restarted.";
                    }
                    else {
                        qDebug() << "Restart failed:"
                            << server.errorString();
                    }
                }
                else if (command == "exit") {
                    server.stopServer();
                    app.quit();
                }
                else {
                    qDebug() << "Commands: send1, send2, stop, start, exit";
                }

                }, Qt::QueuedConnection);

            if (command == "exit")
                break;
        }
        });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (inputThread.joinable()) {
            inputThread.detach();
        }
        });

    return app.exec();
}
