#include "database.h"
#include "user_manager.h"
#include "conversation_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <QVariantMap>
#include <QList>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream in(stdin);
    QTextStream out(stdout);

    // =========================================
    // 1. KET NOI DATABASE
    // =========================================

    Database database;

    QString dbPassword = qEnvironmentVariable("CHAT_DB_PASSWORD");

    if (dbPassword.isEmpty())
    {
        qCritical() << "Chua cau hinh CHAT_DB_PASSWORD!";
        return 1;
    }

    if (!database.connectToMySQL(
            "127.0.0.1",
            3306,
            "chat_tcp",
            "chat_app",
            dbPassword))
    {
        qCritical() << "Ket noi MySQL that bai!";
        qCritical() << database.lastError();

        return 1;
    }

    qDebug() << "Ket noi MySQL thanh cong!";

    // =========================================
    // 2. KHOI TAO CAC BANG DATABASE
    // =========================================

    if (!database.initializeTables())
    {
        qCritical() << "Khoi tao bang database that bai!";
        qCritical() << database.lastError();

        return 1;
    }

    qDebug() << "Khoi tao database thanh cong!";

    // =========================================
    // 3. KHOI TAO CAC MANAGER
    // =========================================

    UserManager users(database);

    ConversationManager conversations(database);

    // =========================================
    // 4. MENU CHUONG TRINH
    // =========================================

    QString choice;

    do
    {
        out << "\n";
        out << "NV2 test menu\n";
        out << "1. Dang ky tai khoan\n";
        out << "2. Dang nhap\n";
        out << "3. Tim user\n";
        out << "4. Danh sach hoi thoai theo user ID\n";
        out << "5. Danh sach thanh vien theo conversation ID\n";
        out << "0. Thoat\n";
        out << "\n";

        out << "Chon: ";
        out.flush();

        choice = in.readLine().trimmed();

        // =====================================
        // CHUC NANG 1: DANG KY TAI KHOAN
        // =====================================

        if (choice == "1")
        {
            QString username;
            QString password;
            QString displayName;
            QString error;

            out << "Username: ";
            out.flush();
            username = in.readLine().trimmed();

            out << "Password: ";
            out.flush();
            password = in.readLine();

            out << "Display name: ";
            out.flush();
            displayName = in.readLine().trimmed();

            bool success = users.registerUser(
                username,
                password,
                displayName,
                &error
            );

            if (success)
            {
                out << "\nDang ky tai khoan thanh cong!\n";
            }
            else
            {
                out << "\nDang ky that bai: "
                    << error
                    << "\n";
            }
        }

        // =====================================
        // CHUC NANG 2: DANG NHAP
        // =====================================

        else if (choice == "2")
        {
            QString username;
            QString password;
            QString error;

            out << "Username: ";
            out.flush();
            username = in.readLine().trimmed();

            out << "Password: ";
            out.flush();
            password = in.readLine();

            QVariantMap user = users.authenticate(
                username,
                password,
                &error
            );

            if (!user.isEmpty())
            {
                out << "\nDang nhap thanh cong!\n";

                out << "Thong tin user:\n";

                out << "ID: "
                    << user.value("id").toString()
                    << "\n";

                out << "Username: "
                    << user.value("username").toString()
                    << "\n";

                out << "Display name: "
                    << user.value("display_name").toString()
                    << "\n";

                out << "Avatar: "
                    << user.value("avatar").toString()
                    << "\n";
            }
            else
            {
                out << "\nDang nhap that bai: "
                    << error
                    << "\n";
            }
        }

        // =====================================
        // CHUC NANG 3: TIM USER
        // =====================================

        else if (choice == "3")
        {
            QString keyword;
            QString error;

            out << "Nhap tu khoa tim user: ";
            out.flush();

            keyword = in.readLine().trimmed();

            QList<QVariantMap> userList;

            bool success = users.findUsers(
                keyword,
                &userList,
                &error
            );

            if (!success)
            {
                out << "Loi tim kiem: "
                    << error
                    << "\n";
            }
            else if (userList.isEmpty())
            {
                out << error << "\n";
            }
            else
            {
                out << "\n";
                out << "===== KET QUA TIM USER =====\n";

                for (const QVariantMap &user : userList)
                {
                    out << "ID: "
                        << user.value("id").toString()
                        << "\n";

                    out << "Username: "
                        << user.value("username").toString()
                        << "\n";

                    out << "Display name: "
                        << user.value("display_name").toString()
                        << "\n";

                    out << "Created at: "
                        << user.value("created_at").toString()
                        << "\n";

                    out << "---------------------------\n";
                }
            }
        }

        // =====================================
// CHUC NANG 4: DANH SACH HOI THOAI
// THEO USER ID
// =====================================
else if (choice == "4")
{
    QString userIdText;

    out << "Nhap User ID: ";
    out.flush();

    userIdText = in.readLine().trimmed();

    bool ok = false;
    int userId = userIdText.toInt(&ok);

    if (!ok || userId <= 0)
    {
        out << "User ID khong hop le!\n";
    }
    else
    {
        QVariantList conversationList =
            conversations.getConversationsByUserId(userId);

        if (conversationList.isEmpty())
        {
            out << "\nKhong tim thay hoi thoai nao "
                   "cua User ID: "
                << userId << "\n";
        }
        else
        {
            out << "\n===== DANH SACH HOI THOAI =====\n";

            for (const QVariant &item : conversationList)
            {
                QVariantMap conversation = item.toMap();

                out << "Conversation ID: "
                    << conversation.value("id").toString()
                    << "\n";

                out << "Ten hoi thoai: "
                    << conversation.value("name").toString()
                    << "\n";

                out << "Ngay tao: "
                    << conversation.value("created_at").toString()
                    << "\n";

                out << "-----------------------------\n";
            }
        }
    }
}
       // =====================================
// CHUC NANG 5: DANH SACH THANH VIEN
// THEO CONVERSATION ID
// =====================================
else if (choice == "5")
{
    QString conversationIdText;

    out << "Nhap Conversation ID: ";
    out.flush();

    conversationIdText = in.readLine().trimmed();

    bool ok = false;
    int conversationId = conversationIdText.toInt(&ok);

    if (!ok || conversationId <= 0)
    {
        out << "Conversation ID khong hop le!\n";
    }
    else
    {
        QVariantList memberList =
            conversations.getMembersByConversationId(
                conversationId
            );

        if (memberList.isEmpty())
        {
            out << "\nKhong tim thay thanh vien "
                   "trong Conversation ID: "
                << conversationId << "\n";
        }
        else
        {
            out << "\n===== DANH SACH THANH VIEN =====\n";

            for (const QVariant &item : memberList)
            {
                QVariantMap member = item.toMap();

                out << "User ID: "
                    << member.value("id").toString()
                    << "\n";

                out << "Username: "
                    << member.value("username").toString()
                    << "\n";

                out << "Display name: "
                    << member.value("display_name").toString()
                    << "\n";

                out << "Avatar: "
                    << member.value("avatar").toString()
                    << "\n";

                out << "-----------------------------\n";
            }
        }
    }
}

        // =====================================
        // CHUC NANG 0: THOAT
        // =====================================

        else if (choice == "0")
        {
            out << "\nThoat chuong trinh.\n";
        }

        // =====================================
        // LUA CHON KHONG HOP LE
        // =====================================

        else
        {
            out << "\nLua chon khong hop le!\n";
        }

    } while (choice != "0");

    // =========================================
    // 5. KET THUC CHUONG TRINH
    // =========================================

    return 0;
}