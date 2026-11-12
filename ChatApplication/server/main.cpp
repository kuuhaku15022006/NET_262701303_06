
#include <QCoreApplication>  
#include <QDebug>           
#include <QTextStream>      
#include <QHostAddress>     
#include <QMetaObject>      
#include <iostream>          
#include <thread>           

#include "ChatServer.h"      

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream input(stdin);    
    QTextStream output(stdout);  

    QHostAddress host;
    int port = 0;

    
    while (true) {

        QString hostInput;
        QString portInput;

        output << "Enter host (ex: 127.0.0.1): ";
        output.flush();
        hostInput = input.readLine().trimmed();

        if (!host.setAddress(hostInput)) {
            qCritical() << "Invalid host address! Try again.";
            continue; 
        }

		output << "Enter port (1->65535): ";
        output.flush();
        portInput = input.readLine().trimmed();

        bool portOk = false;

        int inputPort = portInput.toInt(&portOk);
        //check port
        if (!portOk || inputPort < 1 || inputPort > 65535) {
            qCritical() << "Invalid port! Please enter 1-65535.";
            continue; 
        }

        port = inputPort;

        break;
    }

    ChatServer server;

    QObject::connect(
        &server,
        &ChatServer::serverStarted,
        [](quint16 port) {
            qDebug() << "Server started on port:" << port;
        }
    );

    QObject::connect(
        &server,
        &ChatServer::serverStopped,
        []() {
            qDebug() << "Server stopped";
        }
    );

    QObject::connect(
        &server,
        &ChatServer::clientConnected,
        [](QTcpSocket* socket) {
            qDebug() << "Client connected:"
                << socket->peerAddress().toString();
        }
    );

    QObject::connect(
        &server,
        &ChatServer::clientDisconnected,
        [](QTcpSocket* socket) {
            qDebug() << "Client disconnected:"
                << socket->peerAddress().toString();
        }
    );

    QObject::connect(
        &server,
        &ChatServer::clientMessageReceived,
        [](QTcpSocket* socket, const QByteArray& message) {
            qDebug() << "Received from"
                << socket->peerAddress().toString()
                << ":" << message;
        }
    );

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

            QMetaObject::invokeMethod(
                &app,
                [&, command]() {

                    if (command == "stop") {
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
                        qDebug() << "Commands: stop, start, exit";
                    }
                },
                Qt::QueuedConnection
            );

            if (command == "exit")
                break;
        }
        });

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        [&]() {

            if (inputThread.joinable()) {

                inputThread.detach();
            }
        }
    );

    return app.exec();
}
