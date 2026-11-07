#include "database.h"
#include "user_manager.h"
#include "conversation_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <QVariantMap>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream in(stdin);
    QTextStream out(stdout);

    // Khoi tao Database
    Database database;

    if (!database.open())
    {
        qCritical() << "Khong the ket noi MySQL:"
                   << database.lastError();
        return 1;
    }

    // Khoi tao cac Manager
    UserManager users(database);
    ConversationManager conversations(database);

    // Khoi tao bang
    if (!database.initializeTables())
    {
        qCritical() << "Khong the khoi tao database:"
                   << database.lastError();
        return 1;
    }

    QString choice;

    do
    {
        out << "\nNV2 test menu\n";
        out << "1. Dang ky tai khoan\n";
        out << "2. Dang nhap\n";
        out << "3. Tim user\n";
        out << "4. Danh sach hoi thoai theo user ID\n";
        out << "5. Danh sach thanh vien theo conversation ID\n";
        out << "0. Thoat\n\n";

        out << "Chon: " << Qt::flush;

        choice = in.readLine().trimmed();

        // CHUC NANG 1: DANG KY
        if (choice == "1")
        {
            out << "Username: " << Qt::flush;
            QString username = in.readLine();

            out << "Password: " << Qt::flush;
            QString password = in.readLine();

            out << "Display name: " << Qt::flush;
            QString displayName = in.readLine();

            QString error;

            if (users.registerUser(
                    username,
                    password,
                    displayName,
                    &error))
            {
                qInfo() << "Dang ky thanh cong!";
            }
            else
            {
                qWarning() << "Dang ky that bai:" << error;
            }
        }

        // CHUC NANG 2: DANG NHAP
        else if (choice == "2")
        {
            out << "Username: " << Qt::flush;
            QString username = in.readLine();

            out << "Password: " << Qt::flush;
            QString password = in.readLine();

            QString error;

            QVariantMap user = users.authenticate(
                username,
                password,
                &error
            );

            if (user.isEmpty())
            {
                qWarning() << "Dang nhap that bai:" << error;
            }
            else
            {
                qInfo() << "\nDang nhap thanh cong:";
                qInfo().noquote() << user;
            }
        }

        // CHUC NANG 3
        else if (choice == "3")
        {
            out << "Nhap tu khoa tim user: " << Qt::flush;
            QString keyword = in.readLine();

            // Se bo sung searchUsers()
            out << "Chuc nang tim user.\n";
        }

        // CHUC NANG 4
        else if (choice == "4")
        {
            out << "Nhap user ID: " << Qt::flush;
            QString userId = in.readLine();

            out << "Chuc nang danh sach hoi thoai.\n";
        }

        // CHUC NANG 5
        else if (choice == "5")
        {
            out << "Nhap conversation ID: " << Qt::flush;
            QString conversationId = in.readLine();

            out << "Chuc nang danh sach thanh vien.\n";
        }

        else if (choice == "0")
        {
            out << "Thoat chuong trinh.\n";
        }

        else
        {
            out << "Lua chon khong hop le!\n";
        }

    } while (choice != "0");

    return 0;
}