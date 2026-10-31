#include "database.h"
#include "user_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcessEnvironment>
#include <QTextStream>


UserManager userManager(database);

QTextStream in(stdin);
QTextStream out(stdout);

out << "===== DANG KY TAI KHOAN =====\n";

out << "Username: ";
out.flush();
QString username = in.readLine();

out << "Password: ";
out.flush();
QString password = in.readLine();

out << "Display name: ";
out.flush();
QString displayName = in.readLine();

QString error;

if (userManager.registerUser(
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