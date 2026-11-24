#include "ChatServer.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QMetaObject>
#include <QVariant>
#include <QStringList>
#include <limits>
#include <iostream>
#include <thread>

// Chi duoc bat xu ly tin nhan khi da bat ket noi Database.
#if defined(NV1_MESSAGE_SCHEMA_ALIGNED) && !defined(NV1_WITH_DATABASE)
#error NV1_MESSAGE_SCHEMA_ALIGNED yeu cau NV1_WITH_DATABASE
#endif

#ifdef NV1_WITH_DATABASE
#include "../database/code/database.h"
#include "../database/code/user_manager.h"
#include "../database/code/conversation_manager.h"
#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
#include "../chatServer/MessageManager.h"
#include <QSqlDatabase>
#include <memory>
#include <utility>
#endif
#endif

namespace {
    // NV4 da quy uoc type/request_id/payload, response/event, message_received.
    // success/error va cac key ben trong response.payload la de xuat NV1,
    // can NV4/NV5 xac nhan khi noi GUI (repo chua co quy uoc day du).
    // Tao response theo protocol cua nhom va gui ve dung socket yeu cau.
    void respond(ChatServer& server, QTcpSocket* socket, const QJsonValue& requestId,
        bool success, const QJsonObject& payload = {}, const QString& error = {})
    {
        // Giu nguyen ten truong JSON va request_id de NV4 doi chieu phan hoi.
        const QJsonObject packet{
            { "type", "response" },{ "request_id", requestId },{ "success", success },
            { "payload", payload },{ "error", success ? QJsonValue(QJsonValue::Null) : QJsonValue(error) }
        };
        QByteArray encoded = Protocol::encode(packet);
        // Tra loi ro rang neu phan hoi vuot gioi han kich thuoc frame.
        if (encoded.size() > 1024 * 1024) {
            encoded = Protocol::encode(QJsonObject{
                { "type", "response" },{ "request_id", requestId },{ "success", false },
                { "payload", QJsonObject{} },{ "error", "Phan hoi qua lon; vui long yeu cau it du lieu hon" }
                });
        }
        if (!server.sendToSocket(socket, encoded))
            qWarning() << "Khong the dua phan hoi vao bo dem gui";
    }

    // Kiem tra JSON number co la so nguyen duong hop le cho ID hay khong.
    bool positiveInt(const QJsonValue& value, int& result)
    {
        if (!value.isDouble()) return false;
        result = value.toInt(-1);
        return result > 0;
    }

#if defined(NV1_WITH_DATABASE) && defined(NV1_MESSAGE_SCHEMA_ALIGNED)
    // Kiem tra so nguyen khong am, dung cho offset khi lay lich su.
    bool nonnegativeInt(const QJsonValue& value, int& result)
    {
        if (!value.isDouble()) return false;
        result = value.toInt(-1);
        return result >= 0;
    }
#endif

#ifdef NV1_WITH_DATABASE
    // SQL co the tra BIGINT; kiem tra pham vi truoc khi chuyen sang int cua NV4.
    // Chuyen ID cua Database sang kieu int ma NV4 ho tro, kiem tra tran so.
    bool clientId(const QVariant& value, int& result)
    {
        bool ok = false;
        const qlonglong id = value.toLongLong(&ok);
        if (!ok || id <= 0 || id > (std::numeric_limits<int>::max)()) return false;
        result = static_cast<int>(id);
        return true;
    }
#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
    // Chuyen ID dang chuoi thanh JSON number; tra null neu ID khong hop le.
    QJsonValue numericId(const QString& text)
    {
        int id = 0;
        return clientId(QVariant(text), id) ? QJsonValue(id) : QJsonValue(QJsonValue::Null);
    }

    // Message.h khai bao toJson nhung main chua co implementation.
    // Chuyen doi tai boundary, khong sua class cua NV3.
    // Chuyen doi du lieu Message cua NV3 thanh JSON de tra cho TCPClient.
    QJsonObject messageJson(const Message& message)
    {
        return QJsonObject{
            { "id", numericId(message.id) },
            { "conversation_id", numericId(message.conversationId) },
            { "sender_id", numericId(message.senderId) },
            { "content", message.content },
            { "timestamp", message.timestamp.toUTC().toString(Qt::ISODate) },
            { "reply_to", numericId(message.replyTo) },
            { "forwarded_from", numericId(message.forwardedFrom) }
        };
    }

#endif

    // Tap hop cac manager cua NV2/NV3 de router su dung, khong viet lai nghiep vu.
    class Backend
    {
    public:
        Database database;
        UserManager users{ database };
        ConversationManager conversations{ database };
#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
        std::unique_ptr<MessageManager> messages;
        bool messageReady = false;
#endif

        // Mo ket noi MySQL, khoi tao bang va kiem tra dieu kien tich hop MessageManager.
        bool open()
        {
#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
            const QStringList oldConnections = QSqlDatabase::connectionNames();
#endif
            // Doc cau hinh MySQL tu bien moi truong, khong ghi cung mat khau.
            bool validPort = false;
            const int port = qEnvironmentVariable("CHAT_DB_PORT", "3306").toInt(&validPort);
            const QString password = qEnvironmentVariable("CHAT_DB_PASSWORD");
            if (!validPort || port < 1 || port > 65535 || password.isEmpty()) {
                qCritical() << "Can cai dat CHAT_DB_PASSWORD va CHAT_DB_PORT hop le";
                return false;
            }
            if (!database.connectToMySQL(qEnvironmentVariable("CHAT_DB_HOST", "127.0.0.1"),
                port, qEnvironmentVariable("CHAT_DB_NAME", "chat_tcp"),
                qEnvironmentVariable("CHAT_DB_USER", "chat_app"), password)) {
                qCritical() << "Khong the ket noi co so du lieu. Kiem tra MySQL, QODBC va cac bien moi truong.";
                return false;
            }
            if (!database.initializeTables()) {
                qCritical() << "Khong the khoi tao cac bang co so du lieu";
                return false;
            }
#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
            // Database cua NV2 chua cung cap ham lay QSqlDatabase. Tim dung ket noi
            // vua tao, khong dung ket noi mac dinh vi NV2 dat ten rieng.
            QStringList newConnections = QSqlDatabase::connectionNames();
            for (const auto& name : oldConnections) newConnections.removeAll(name);
            if (newConnections.size() != 1) {
                qCritical() << "Khong the xac dinh ket noi co so du lieu cua NV2";
                return false;
            }
            QSqlDatabase sql = QSqlDatabase::database(newConnections.first(), false);
            if (!sql.isOpen()) return false;
            // Chi bat khi NV2/NV3 da thong nhat ID tin nhan dang so, ten cot va
            // cach luu tin nhan. Chi kiem tra cau truc bang, khong tu sua schema.
            if (!database.execute("SET time_zone = '+00:00'")) {
                qCritical() << "Khong the thiet lap mui gio cho phien co so du lieu";
                return false;
            }
            auto probe = database.query(
                "SELECT id, conversation_id, sender_id, content, reply_to, forwarded_from FROM messages LIMIT 0");
            if (!probe.isActive()) {
                qCritical() << "Cau truc bang tin nhan chua dong bo; tat NV1_MESSAGE_SCHEMA_ALIGNED cho den khi sua xong.";
                return false;
            }
            probe.finish();
            auto candidate = std::make_unique<MessageManager>(sql);
            // Thu truy van doc cua NV3 de kiem tra ca ten cot thoi gian.
            QString error;
            candidate->get_history(QStringLiteral("0"), 1, 0, &error);
            if (!error.isEmpty()) {
                qCritical() << "Truy van MessageManager khong khop cau truc bang; tat NV1_MESSAGE_SCHEMA_ALIGNED cho den khi sua xong.";
                return false;
            }
            // Doc thanh cong chua chung minh NV3 tao ID dang so khi INSERT.
            // Chi bat macro sau khi ca nhom da thong nhat cach tich hop.
            messages = std::move(candidate);
            messageReady = true;
            qInfo() << "Da bat xu ly tin nhan; can kiem tra tich hop toan bo luong voi NV3.";
#else
            qWarning() << "Tam tat xu ly tin nhan cho den khi NV2/NV3 dong bo cau truc bang.";
#endif
            return true;
        }

#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
        // Kiem tra User ID co thuoc hoi thoai truoc khi cho phep doc/gui tin nhan.
        bool isMember(int userId, int conversationId)
        {
            const QVariantList members = conversations.getMembersByConversationId(conversationId);
            if (!database.lastError().isEmpty()) return false;
            for (const auto& entry : members) {
                int memberId = 0;
                if (clientId(entry.toMap().value("id"), memberId) && memberId == userId)
                    return true;
            }
            return false;
        }

        // Gui event tin nhan moi cho cac thanh vien online, tru nguoi gui.
        void publish(ChatServer& server, int conversationId, int senderId, const Message& message)
        {
            const QByteArray event = Protocol::encode(QJsonObject{
                { "type", "event" },{ "event", "message_received" },{ "payload", messageJson(message) }
                });
            const auto members = conversations.getMembersByConversationId(conversationId);
            if (!database.lastError().isEmpty()) {
                qWarning() << "Khong the lay danh sach thanh vien de gui su kien";
                return;
            }
            for (const auto& entry : members) {
                int memberId = 0;
                if (!clientId(entry.toMap().value("id"), memberId) || memberId == senderId)
                    continue;
                // Nguoi nhan offline la binh thuong vi tin nhan da duoc luu.
                server.sendToUser(QString::number(memberId), event);
            }
        }
#endif
    };
#endif

    // Router: kiem tra request, xac thuc va phan quyen, goi manager tuong ung.
    void handleRequest(ChatServer& server, QTcpSocket* socket, const QJsonObject& request
#ifdef NV1_WITH_DATABASE
        , Backend& backend
#endif
    )
    {
        // Kiem tra cau truc goi tin truoc khi goi cac manager.
        const QJsonValue rawId = request.value("request_id");
        int requestNumber = 0;
        const bool validId = positiveInt(rawId, requestNumber);
        const QJsonValue id = validId ? QJsonValue(requestNumber) : QJsonValue(QJsonValue::Null);
        const QString type = request.value("type").toString();
        const auto fail = [&](const QString& error) { respond(server, socket, id, false, {}, error); };
        const auto ok = [&](const QJsonObject& data = {}) { respond(server, socket, id, true, data); };

        if (!validId || type.isEmpty() || !request.value("payload").isObject()) {
            fail("Yeu cau khong hop le: can type, request_id la so nguyen duong va payload la JSON object");
            return;
        }
        const QJsonObject payload = request.value("payload").toObject();
        if (type != "login" && type != "register" && type != "get_conversations" &&
            type != "get_history" && type != "send_message" && type != "forward_message") {
            fail("Loai yeu cau khong duoc ho tro");
            return;
        }

        // Socket moi chua co identity. Khong tin sender_id/user_id do client tu gui.
        const QString currentUser = server.getUserId(socket);
        // Hai nghiep vu nay cho phep socket chua xac thuc gui yeu cau.
        if (type == "login" || type == "register") {
            if (!currentUser.isEmpty()) { fail("Tai khoan da dang nhap tren ket noi nay"); return; }
            const QString username = payload.value("username").toString().trimmed();
            const QString password = payload.value("password").toString();
            const QString displayName = payload.value("display_name").toString().trimmed();
            if (username.isEmpty() || username.size() > 50 || password.isEmpty() || password.size() > 4096 ||
                (type == "register" && (displayName.isEmpty() || displayName.size() > 100 || password.size() < 8))) {
                fail("Thong tin tai khoan khong hop le"); return;
            }
#ifdef NV1_WITH_DATABASE
            QString error;
            if (type == "register") {
                if (!backend.users.registerUser(username, password, displayName, &error)) {
                    // Khong tra SQL/credential details ve mang.
                    fail("Dang ky tai khoan that bai"); return;
                }
                ok(); // Register khong tu dong login.
                return;
            }
            // Xac thuc qua UserManager cua NV2, khong tu kiem tra mat khau.
            const QVariantMap user = backend.users.authenticate(username, password, &error);
            int userId = 0;
            if (user.isEmpty() || !clientId(user.value("id"), userId)) {
                fail("Ten dang nhap hoac mat khau khong chinh xac"); return;
            }
            // Chi gan User ID vao socket sau khi xac thuc thanh cong.
            if (!server.registerUser(QString::number(userId), socket)) {
                fail("Tai khoan da dang nhap hoac socket khong kha dung"); return;
            }
            // Chi tra cac truong an toan; id phai la JSON number cho NV4.
            QJsonObject publicUser{ { "id", userId },{ "username", user.value("username").toString() },
                { "display_name", user.value("display_name").toString() },
                { "avatar", QJsonValue::fromVariant(user.value("avatar")) } };
            ok(QJsonObject{ { "user", publicUser } });
#else
            fail("Co so du lieu chua duoc bat (che do chi kiem thu mang)");
#endif
            return;
        }

        // Cac nghiep vu con lai bat buoc co User ID da xac thuc.
        bool userOk = false;
        const int userId = currentUser.toInt(&userOk);
        if (!userOk || userId <= 0) { fail("Vui long dang nhap truoc khi thuc hien yeu cau"); return; }

#ifdef NV1_WITH_DATABASE
        // Lay danh sach hoi thoai cua user hien tai tu ConversationManager.
        if (type == "get_conversations") {
            const auto entries = backend.conversations.getConversationsByUserId(userId);
            if (!backend.database.lastError().isEmpty()) { fail("Khong the lay danh sach hoi thoai"); return; }
            QJsonArray result;
            for (const auto& entry : entries) {
                const QVariantMap item = entry.toMap();
                int conversationId = 0;
                if (!clientId(item.value("id"), conversationId)) { fail("ID hoi thoai vuot pham vi so nguyen cua client"); return; }
                result.append(QJsonObject{ { "id", conversationId },{ "name", QJsonValue::fromVariant(item.value("name")) },
                    { "created_at", QJsonValue::fromVariant(item.value("created_at")) } });
            }
            ok(QJsonObject{ { "conversations", result } });
            return;
        }

#ifdef NV1_MESSAGE_SCHEMA_ALIGNED
        // Chi xu ly tin nhan khi schema va MessageManager da tuong thich.
        if (!backend.messageReady || !backend.messages) {
            fail("Chua the xu ly tin nhan: cau truc co so du lieu chua dong bo"); return;
        }
        int conversationId = 0;
        const QString conversationKey = type == "forward_message" ? "target_conversation_id" : "conversation_id";
        if (!positiveInt(payload.value(conversationKey), conversationId)) { fail("ID hoi thoai khong hop le"); return; }
        if (!backend.isMember(userId, conversationId)) { fail("Hoi thoai khong ton tai hoac ban khong co quyen truy cap"); return; }
        const QString conv = QString::number(conversationId);
        const QString sender = QString::number(userId);
        QString error;
        MessageManager& messages = *backend.messages;
        // Doc lich su theo tung trang, gioi han so luong de tranh phan hoi qua lon.
        if (type == "get_history") {
            int limit = 50, offset = 0;
            if ((payload.contains("limit") && !positiveInt(payload.value("limit"), limit)) ||
                (payload.contains("offset") && !nonnegativeInt(payload.value("offset"), offset)) || limit > 100) {
                fail("limit hoac offset khong hop le (limit tu 1 den 100)"); return;
            }
            const QList<Message> history = messages.get_history(conv, limit, offset, &error);
            if (!error.isEmpty()) { fail("Khong the lay lich su tin nhan"); return; }
            QJsonArray items;
            for (const Message& message : history) {
                const auto item = messageJson(message);
                if (item.value("id").isNull() || item.value("sender_id").isNull() ||
                    item.value("conversation_id").toInt() != conversationId ||
                    (!message.replyTo.isEmpty() && item.value("reply_to").isNull()) ||
                    (!message.forwardedFrom.isEmpty() && item.value("forwarded_from").isNull())) {
                    fail("Du lieu tin nhan khong khop dinh dang ID cua client");
                    return;
                }
                items.append(item);
            }
            ok(QJsonObject{ { "messages", items } });
            return;
        }

        QString newId;
        bool saved = false;
        // Phan biet gui tin nhan thuong va tra loi tin nhan goc.
        if (type == "send_message") {
            const QString content = payload.value("content").toString();
            if (content.trimmed().isEmpty() || content.size() > 16000) { fail("Noi dung tin nhan khong hop le"); return; }
            const QJsonValue reply = payload.value("reply_to");
            if (reply.isNull() || reply.isUndefined()) {
                saved = messages.send_message(conv, sender, content, &newId, &error);
            }
            else {
                int replyId = 0;
                if (!positiveInt(reply, replyId)) { fail("ID tin nhan tra loi (reply_to) khong hop le"); return; }
                Message original;
                if (!messages.get_message(QString::number(replyId), original, &error) || original.conversationId != conv) {
                    fail("Khong tim thay tin nhan can tra loi trong hoi thoai"); return;
                }
                saved = messages.reply_message(conv, sender, content, QString::number(replyId), &newId, &error);
            }
            // Chuyen tiep: kiem tra quyen truy cap ca hoi thoai nguon va dich.
        }
        else if (type == "forward_message") {
            int messageId = 0;
            if (!positiveInt(payload.value("message_id"), messageId)) { fail("ID tin nhan khong hop le"); return; }
            Message original;
            if (!messages.get_message(QString::number(messageId), original, &error)) { fail("Khong tim thay tin nhan"); return; }
            int sourceId = 0;
            if (!clientId(QVariant(original.conversationId), sourceId) || !backend.isMember(userId, sourceId)) {
                fail("Tin nhan khong ton tai hoac ban khong co quyen truy cap"); return;
            }
            saved = messages.forward_message(QString::number(messageId), conv, sender, &newId, &error);
        }
        if (!saved) { fail("Khong the luu tin nhan"); return; }
        // Tin nhan da luu thanh cong. Khong tra loi that bai sai su that de tranh
        // client gui trung tin khi buoc doc lai hoac gui su kien gap loi.
        const QJsonValue messageId = numericId(newId);
        QJsonObject result{ { "message_id", messageId } };
        Message savedMessage;
        error.clear();
        bool eventReady = false;
        if (!messageId.isNull() && messages.get_message(newId, savedMessage, &error)) {
            const QJsonObject item = messageJson(savedMessage);
            eventReady = item.value("id") == messageId &&
                item.value("conversation_id").toInt() == conversationId &&
                item.value("sender_id").toInt() == userId &&
                (savedMessage.replyTo.isEmpty() || !item.value("reply_to").isNull()) &&
                (savedMessage.forwardedFrom.isEmpty() || !item.value("forwarded_from").isNull());
            if (eventReady) result.insert("message", item);
        }
        // Nguoi gui nhan tin da luu trong response, khong nhan them
        // event message_received trung lap. Can NV4/NV5 thong nhat payload nay.
        ok(result);
        if (eventReady)
            backend.publish(server, conversationId, userId, savedMessage);
        else
            qWarning() << "Da luu tin nhan nhung khong the tao su kien; kiem tra ID va cau truc bang voi NV3";
#else
        fail("Chua the xu ly tin nhan: NV2/NV3 chua dong bo cau truc bang");
#endif
#else
        fail("Co so du lieu chua duoc bat (che do chi kiem thu mang)");
#endif
    }
} // ket thuc namespace

// Diem bat dau: doc cau hinh, khoi tao Server, noi callback va chay event loop.
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ChatServer");
    // Ho tro nhap IP/cong qua tham so dong lenh hoac console.
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ "host", "Dia chi IP de Server lang nghe", "ip" });
    parser.addOption({ "port", "Cong TCP", "port" });
    parser.addOption({ "no-console", "Chay khong can nhap lenh start/stop/exit" });
    parser.process(app);

    QHostAddress host;
    int port = 0;
    const bool hasAddress = parser.isSet("host") || parser.isSet("port");
    if (hasAddress || parser.isSet("no-console")) {
        bool portOk = false;
        port = parser.value("port").toInt(&portOk);
        if (!host.setAddress(parser.value("host")) || !portOk || port < 1 || port > 65535) {
            qCritical() << "Vui long cung cap --host va --port (1-65535)";
            return 1;
        }
    }
    else {
        std::string input;
        while (true) {
            std::cout << "Nhap dia chi IP (vi du: 127.0.0.1): " << std::flush;
            if (!std::getline(std::cin, input)) return 1;
            if (!host.setAddress(QString::fromStdString(input).trimmed())) {
                qWarning() << "Dia chi IP khong hop le"; continue;
            }
            std::cout << "Nhap cong (1-65535): " << std::flush;
            if (!std::getline(std::cin, input)) return 1;
            bool ok = false;
            port = QString::fromStdString(input).trimmed().toInt(&ok);
            if (ok && port >= 1 && port <= 65535) break;
            qWarning() << "Cong khong hop le";
        }
    }

#ifdef NV1_WITH_DATABASE
    Backend backend;
    if (!backend.open()) return 1;
#else
    qWarning() << "Che do chi kiem thu mang: chua bat xac thuc va xu ly tin nhan that";
#endif
    // Khoi tao tang TCP sau khi backend (neu bat) da san sang.
    ChatServer server;
    QObject::connect(&server, &ChatServer::serverStarted, &app, [](quint16 p) {
        qInfo() << "Server da khoi dong tren cong:" << p;
        });
    QObject::connect(&server, &ChatServer::serverStopped, &app, [] { qInfo() << "Server da dung"; });
    QObject::connect(&server, &ChatServer::clientConnected, &app, [&server](QTcpSocket*) {
        qInfo() << "Client da ket noi. So ket noi hien tai:" << server.clientCount();
        });
    QObject::connect(&server, &ChatServer::clientDisconnected, &app, [&server](QTcpSocket*) {
        qInfo() << "Client da ngat ket noi. So ket noi hien tai:" << server.clientCount();
        });
    QObject::connect(&server, &ChatServer::clientRequestReceived, &app,
        [&](QTcpSocket* socket, const QJsonObject& request) {
            handleRequest(server, socket, request
#ifdef NV1_WITH_DATABASE
                , backend
#endif
            );
        });
    if (!server.startServer(host, port)) {
        qCritical() << "Loi Server:" << server.errorString();
        return 1;
    }
    // Khong dang ky test1/test2 khi ket noi. Chi bind ID sau authenticate.
    // Console rieng, tat ca thao tac Qt socket duoc dua ve Qt event loop.
    std::thread inputThread;
    if (!parser.isSet("no-console")) {
        inputThread = std::thread([&]() {
            std::string line;
            while (std::getline(std::cin, line)) {
                const QString command = QString::fromStdString(line).trimmed();
                QMetaObject::invokeMethod(&app, [&, command] {
                    if (command == "stop") server.stopServer();
                    else if (command == "start") {
                        if (!server.startServer(host, port)) qWarning() << server.errorString();
                    }
                    else if (command == "exit") {
                        server.stopServer(); app.quit();
                    }
                    else qInfo() << "Lenh ho tro: stop, start, exit";
                    }, Qt::QueuedConnection);
                if (command == "exit") return;
            }
            QMetaObject::invokeMethod(&app, [&] { server.stopServer(); app.quit(); }, Qt::QueuedConnection);
            });
    }
    // Bat dau event loop de Qt xu ly su kien socket va cac lenh console.
    const int result = app.exec();
    // Exit binh thuong qua lenh exit/EOF; thread da thoat, khong detach.
    if (inputThread.joinable()) inputThread.join();
    server.stopServer();
    return result;
}
