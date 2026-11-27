
#include "ChatServer.h"
#include "../database/code/user_manager.h"
#include "../database/code/conversation_manager.h"
#include "../chatServer/MessageManager.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QVariantMap>

namespace {
QString jsonValToString(const QJsonValue& val)
{
    if (val.isString()) return val.toString().trimmed();
    if (val.isDouble()) return QString::number(val.toVariant().toLongLong());
    return QString();
}
}

ChatServer::ChatServer(QObject* parent)
    : QObject(parent)
{
    // tao server
    server = new QTcpServer(this);

    connectionManager = new ConnectionManager(this);

    connect(server, &QTcpServer::newConnection,
        this, &ChatServer::handleNewConnection);
}

ChatServer::~ChatServer()
{
    stopServer();
}

void ChatServer::setManagers(UserManager* userMgr,
                             ConversationManager* convMgr,
                             MessageManager* msgMgr)
{
    userManager = userMgr;
    conversationManager = convMgr;
    messageManager = msgMgr;
}

bool ChatServer::registerUser(
    const QString& userId, QTcpSocket* socket)
{
    return connectionManager->addConnection(userId, socket);
}

bool ChatServer::sendToUser(
    const QString& userId, const QByteArray& message)
{
    return connectionManager->sendToUser(userId, message);
}

bool ChatServer::startServer(const QHostAddress& host, int port)
{
    if (server->isListening())
        return true;

    if (port < 1 || port > 65535)
        return false;

    if (!server->listen(host, static_cast<quint16>(port)))
        return false;

    emit serverStarted(static_cast<quint16>(port));

    return true;
}

QString ChatServer::errorString() const
{
    return server->errorString();
}


void ChatServer::stopServer()
{
    if (!server->isListening() && clients.isEmpty())
        return;

    server->close();

    while (!clients.isEmpty()) {

        ClientHandler* client = clients.takeLast();

        QTcpSocket* socket = client->getSocket();

        // Ngắt các signal callback từ ClientHandler về ChatServer
        disconnect(client, nullptr, this, nullptr);

        if (socket) {
            connectionManager->removeSocket(socket);
            socket->abort();
        }

        client->deleteLater();
    }

    emit serverStopped();
}


bool ChatServer::isRunning() const
{
    return server->isListening();
}

int ChatServer::clientCount() const
{
    return clients.size();
}

void ChatServer::handleNewConnection()
{
    while (server->hasPendingConnections()) {

        QTcpSocket* socket = server->nextPendingConnection();

        ClientHandler* client = new ClientHandler(socket, this);

        clients.append(client);

        connect(client, &ClientHandler::messageReceived,
            this, [this](ClientHandler* handler,
                const QByteArray& message) {

                    QJsonObject request;
                    QString error;

                    if (!Protocol::decode(message, request, &error)) {
                        qWarning() << "Invalid JSON:" << error;
                        QJsonObject errRes;
                        errRes.insert(QStringLiteral("type"), QStringLiteral("response"));
                        errRes.insert(QStringLiteral("request_id"), 0);
                        errRes.insert(QStringLiteral("ok"), false);
                        errRes.insert(QStringLiteral("error"), QStringLiteral("Invalid JSON: ") + error);
                        errRes.insert(QStringLiteral("payload"), QJsonObject());
                        sendJsonToSocket(handler->getSocket(), errRes);
                        return;
                    }

                    emit clientMessageReceived(
                        handler->getSocket(), message);

                    emit clientRequestReceived(
                        handler->getSocket(), request);

                    routeRequest(handler->getSocket(), request);
            });

        connect(client, &ClientHandler::clientDisconnected,
            this, [this](ClientHandler* handler) {

                QTcpSocket* socket = handler->getSocket();

                connectionManager->removeSocket(socket);

                clients.removeOne(handler);

                emit clientDisconnected(socket);

                handler->deleteLater();
            });

        emit clientConnected(socket);
    }
}

void ChatServer::sendJsonToSocket(QTcpSocket* socket, const QJsonObject& obj)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;
    QByteArray data = Protocol::encode(obj) + '\n';
    socket->write(data);
}

void ChatServer::routeRequest(QTcpSocket* socket, const QJsonObject& request)
{
    if (!userManager && !conversationManager && !messageManager)
        return;

    const QString type = request.value(QStringLiteral("type")).toString().trimmed();
    const int requestId = request.value(QStringLiteral("request_id")).toInt(0);
    const QJsonObject payload = request.value(QStringLiteral("payload")).toObject();

    auto sendResponse = [this, socket, requestId, type](bool ok,
                                                        const QJsonObject& resPayload,
                                                        const QString& errorMsg = QString()) {
        QJsonObject res;
        res.insert(QStringLiteral("type"), QStringLiteral("response"));
        res.insert(QStringLiteral("request_id"), requestId);
        res.insert(QStringLiteral("request_type"), type);
        res.insert(QStringLiteral("ok"), ok);
        res.insert(QStringLiteral("payload"), resPayload);
        res.insert(QStringLiteral("error"), errorMsg);
        sendJsonToSocket(socket, res);
    };

    // 1. REGISTER
    if (type == QStringLiteral("register")) {
        if (!userManager) {
            sendResponse(false, {}, QStringLiteral("UserManager chua san sang"));
            return;
        }
        const QString username = payload.value(QStringLiteral("username")).toString();
        const QString password = payload.value(QStringLiteral("password")).toString();
        const QString displayName = payload.value(QStringLiteral("display_name")).toString();
        QString errorText;
        const bool ok = userManager->registerUser(username, password, displayName, &errorText);
        QJsonObject resPayload;
        if (ok) {
            resPayload.insert(QStringLiteral("username"), username.trimmed());
            resPayload.insert(QStringLiteral("display_name"), displayName.trimmed());
        }
        sendResponse(ok, resPayload, errorText);
        return;
    }

    // 2. LOGIN
    if (type == QStringLiteral("login")) {
        if (!userManager) {
            sendResponse(false, {}, QStringLiteral("UserManager chua san sang"));
            return;
        }
        const QString username = payload.value(QStringLiteral("username")).toString();
        const QString password = payload.value(QStringLiteral("password")).toString();
        QString errorText;
        const QVariantMap user = userManager->authenticate(username, password, &errorText);
        if (user.isEmpty()) {
            sendResponse(false, {}, errorText.isEmpty() ? QStringLiteral("Dang nhap that bai") : errorText);
            return;
        }

        const int userIdInt = user.value(QStringLiteral("id")).toInt();
        const QString userIdStr = user.value(QStringLiteral("id")).toString();
        if (conversationManager) {
            conversationManager->ensureDefaultConversationsForUser(userIdInt);
        }
        connectionManager->addConnection(userIdStr, socket);

        QJsonObject resPayload;
        resPayload.insert(QStringLiteral("id"), userIdInt);
        resPayload.insert(QStringLiteral("user_id"), userIdStr);
        resPayload.insert(QStringLiteral("username"), user.value(QStringLiteral("username")).toString());
        resPayload.insert(QStringLiteral("display_name"), user.value(QStringLiteral("display_name")).toString());
        resPayload.insert(QStringLiteral("avatar"), user.value(QStringLiteral("avatar")).toString());
        sendResponse(true, resPayload);
        return;
    }

    // 3. GET CONVERSATIONS
    if (type == QStringLiteral("get_conversations")) {
        if (!conversationManager) {
            sendResponse(false, {}, QStringLiteral("ConversationManager chua san sang"));
            return;
        }
        QString userIdStr = connectionManager->getUserId(socket);
        if (userIdStr.isEmpty())
            userIdStr = jsonValToString(payload.value(QStringLiteral("user_id")));
        const int userIdInt = userIdStr.toInt();
        if (userIdInt <= 0) {
            sendResponse(false, {}, QStringLiteral("Chua dang nhap"));
            return;
        }

        conversationManager->ensureDefaultConversationsForUser(userIdInt);
        const QVariantList convList = conversationManager->getConversationsByUserId(userIdInt);
        QJsonArray arr;
        for (const QVariant& item : convList) {
            const QVariantMap m = item.toMap();
            QJsonObject cObj;
            cObj.insert(QStringLiteral("id"), m.value(QStringLiteral("id")).toInt());
            cObj.insert(QStringLiteral("conversation_id"), m.value(QStringLiteral("id")).toString());
            cObj.insert(QStringLiteral("name"), m.value(QStringLiteral("name")).toString());
            cObj.insert(QStringLiteral("created_at"), m.value(QStringLiteral("created_at")).toString());
            arr.append(cObj);
        }
        QJsonObject resPayload;
        resPayload.insert(QStringLiteral("conversations"), arr);
        sendResponse(true, resPayload);
        return;
    }

    // Helper: tra cứu tên hiển thị của các thành viên trong hội thoại
    auto buildMemberNameMap = [this](int convIdInt) -> QHash<QString, QString> {
        QHash<QString, QString> map;
        if (!conversationManager || convIdInt <= 0)
            return map;
        const QVariantList members = conversationManager->getMembersByConversationId(convIdInt);
        for (const QVariant& item : members) {
            const QVariantMap m = item.toMap();
            const QString idStr = m.value(QStringLiteral("id")).toString();
            QString name = m.value(QStringLiteral("display_name")).toString();
            if (name.isEmpty())
                name = m.value(QStringLiteral("username")).toString();
            if (!idStr.isEmpty() && !name.isEmpty())
                map.insert(idStr, name);
        }
        return map;
    };

    // 4. GET HISTORY
    if (type == QStringLiteral("get_history")) {
        if (!messageManager) {
            sendResponse(false, {}, QStringLiteral("MessageManager chua san sang"));
            return;
        }
        const QString convId = jsonValToString(payload.value(QStringLiteral("conversation_id")));
        const int limit = payload.value(QStringLiteral("limit")).toInt(50);
        const int offset = payload.value(QStringLiteral("offset")).toInt(0);
        QString errorText;
        const QList<Message> history = messageManager->get_history(convId, limit, offset, &errorText);
        if (!errorText.isEmpty()) {
            sendResponse(false, {}, errorText);
            return;
        }

        const QHash<QString, QString> memberNames = buildMemberNameMap(convId.toInt());
        QHash<QString, QString> contentById;
        for (const Message& m : history) {
            contentById.insert(m.id, m.content);
        }

        QJsonArray arr;
        for (const Message& m : history) {
            QJsonObject mObj = m.toJson();
            if (memberNames.contains(m.senderId)) {
                mObj.insert(QStringLiteral("sender_name"), memberNames.value(m.senderId));
            }
            if (!m.replyTo.isEmpty()) {
                QString preview = contentById.value(m.replyTo);
                if (preview.isEmpty()) {
                    Message orig;
                    if (messageManager->get_message(m.replyTo, orig))
                        preview = orig.content;
                }
                if (!preview.isEmpty())
                    mObj.insert(QStringLiteral("reply_preview"), preview);
            }
            arr.append(mObj);
        }

        QJsonObject resPayload;
        resPayload.insert(QStringLiteral("conversation_id"), convId);
        resPayload.insert(QStringLiteral("messages"), arr);
        sendResponse(true, resPayload);
        return;
    }

    // 5. SEND MESSAGE (bao gồm cả Reply)
    if (type == QStringLiteral("send_message")) {
        if (!messageManager) {
            sendResponse(false, {}, QStringLiteral("MessageManager chua san sang"));
            return;
        }
        const QString convId = jsonValToString(payload.value(QStringLiteral("conversation_id")));
        const QString content = payload.value(QStringLiteral("content")).toString();
        const QString replyTo = jsonValToString(payload.value(QStringLiteral("reply_to")));
        QString senderId = connectionManager->getUserId(socket);
        if (senderId.isEmpty())
            senderId = jsonValToString(payload.value(QStringLiteral("sender_id")));

        if (senderId.isEmpty()) {
            sendResponse(false, {}, QStringLiteral("Chua xac thuc nguoi gui"));
            return;
        }

        QString newMessageId;
        QString errorText;
        bool ok = false;
        if (!replyTo.isEmpty() && replyTo != QStringLiteral("-1") && replyTo != QStringLiteral("0")) {
            ok = messageManager->reply_message(convId, senderId, content, replyTo, &newMessageId, &errorText);
        } else {
            ok = messageManager->send_message(convId, senderId, content, &newMessageId, &errorText);
        }

        if (!ok) {
            sendResponse(false, {}, errorText);
            return;
        }

        Message saved;
        messageManager->get_message(newMessageId, saved);
        QJsonObject msgObj = saved.toJson();
        const QHash<QString, QString> memberNames = buildMemberNameMap(convId.toInt());
        if (memberNames.contains(senderId)) {
            msgObj.insert(QStringLiteral("sender_name"), memberNames.value(senderId));
        }
        if (!saved.replyTo.isEmpty()) {
            Message orig;
            if (messageManager->get_message(saved.replyTo, orig))
                msgObj.insert(QStringLiteral("reply_preview"), orig.content);
        }

        sendResponse(true, msgObj);

        // Phát event message_received tới các thành viên trong hội thoại
        QJsonObject eventObj;
        eventObj.insert(QStringLiteral("type"), QStringLiteral("event"));
        eventObj.insert(QStringLiteral("event"), QStringLiteral("message_received"));
        eventObj.insert(QStringLiteral("payload"), msgObj);
        const QByteArray encodedEvent = Protocol::encode(eventObj);

        bool sentToSender = false;
        if (conversationManager) {
            const QVariantList members = conversationManager->getMembersByConversationId(convId.toInt());
            for (const QVariant& item : members) {
                const QString memberId = item.toMap().value(QStringLiteral("id")).toString();
                if (!memberId.isEmpty()) {
                    connectionManager->sendToUser(memberId, encodedEvent);
                    if (memberId == senderId)
                        sentToSender = true;
                }
            }
        }
        if (!sentToSender) {
            connectionManager->sendToUser(senderId, encodedEvent);
        }
        return;
    }

    // 6. FORWARD MESSAGE
    if (type == QStringLiteral("forward_message")) {
        if (!messageManager) {
            sendResponse(false, {}, QStringLiteral("MessageManager chua san sang"));
            return;
        }
        const QString msgId = jsonValToString(payload.value(QStringLiteral("message_id")));
        const QString targetConvId = jsonValToString(payload.value(QStringLiteral("target_conversation_id")));
        QString senderId = connectionManager->getUserId(socket);
        if (senderId.isEmpty())
            senderId = jsonValToString(payload.value(QStringLiteral("sender_id")));

        if (senderId.isEmpty()) {
            sendResponse(false, {}, QStringLiteral("Chua xac thuc nguoi gui"));
            return;
        }

        QString newMessageId;
        QString errorText;
        if (!messageManager->forward_message(msgId, targetConvId, senderId, &newMessageId, &errorText)) {
            sendResponse(false, {}, errorText);
            return;
        }

        Message saved;
        messageManager->get_message(newMessageId, saved);
        QJsonObject msgObj = saved.toJson();
        const QHash<QString, QString> memberNames = buildMemberNameMap(targetConvId.toInt());
        if (memberNames.contains(senderId)) {
            msgObj.insert(QStringLiteral("sender_name"), memberNames.value(senderId));
        }

        sendResponse(true, msgObj);

        QJsonObject eventObj;
        eventObj.insert(QStringLiteral("type"), QStringLiteral("event"));
        eventObj.insert(QStringLiteral("event"), QStringLiteral("message_received"));
        eventObj.insert(QStringLiteral("payload"), msgObj);
        const QByteArray encodedEvent = Protocol::encode(eventObj);

        bool sentToSender = false;
        if (conversationManager) {
            const QVariantList members = conversationManager->getMembersByConversationId(targetConvId.toInt());
            for (const QVariant& item : members) {
                const QString memberId = item.toMap().value(QStringLiteral("id")).toString();
                if (!memberId.isEmpty()) {
                    connectionManager->sendToUser(memberId, encodedEvent);
                    if (memberId == senderId)
                        sentToSender = true;
                }
            }
        }
        if (!sentToSender) {
            connectionManager->sendToUser(senderId, encodedEvent);
        }
        return;
    }

    // 7. SEARCH USERS
    if (type == QStringLiteral("search_users")) {
        if (!userManager) {
            sendResponse(false, {}, QStringLiteral("UserManager chua san sang"));
            return;
        }
        const QString keyword = payload.value(QStringLiteral("keyword")).toString();
        QList<QVariantMap> users;
        QString errorText;
        const bool ok = userManager->findUsers(keyword, &users, &errorText);
        QJsonArray arr;
        for (const QVariantMap& u : users) {
            QJsonObject uObj;
            uObj.insert(QStringLiteral("id"), u.value(QStringLiteral("id")).toInt());
            uObj.insert(QStringLiteral("username"), u.value(QStringLiteral("username")).toString());
            uObj.insert(QStringLiteral("display_name"), u.value(QStringLiteral("display_name")).toString());
            uObj.insert(QStringLiteral("avatar"), u.value(QStringLiteral("avatar")).toString());
            arr.append(uObj);
        }
        QJsonObject resPayload;
        resPayload.insert(QStringLiteral("users"), arr);
        sendResponse(ok, resPayload, ok ? QString() : errorText);
        return;
    }

    sendResponse(false, {}, QStringLiteral("Unknown request type: ") + type);
}
