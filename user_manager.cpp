#include "user_manager.h"

#include <QCryptographicHash>
#include <QSqlQuery>
#include <QUuid>

// Constructor
UserManager::UserManager(Database &database)
    : m_database(database)
{
}

// 1. Tao Salt ngau nhien
QString UserManager::createSalt()
{
    return QUuid::createUuid()
        .toString(QUuid::WithoutBraces);
}

// 2. Bam mat khau
QString UserManager::hashPassword(
    const QString &password,
    const QString &salt
)
{
    QByteArray input = (salt + password).toUtf8();

    QByteArray hash = QCryptographicHash::hash(
        input,
        QCryptographicHash::Sha256
    );

    return QString::fromLatin1(hash.toHex());
}

// 3. Dang ky tai khoan
bool UserManager::registerUser(
    const QString &username,
    const QString &password,
    const QString &displayName,
    QString *errorText
)
{
    // Buoc 1: Kiem tra du lieu
    QString cleanUsername = username.trimmed();
    QString cleanDisplayName = displayName.trimmed();

    if (cleanUsername.isEmpty() ||
        password.isEmpty() ||
        cleanDisplayName.isEmpty())
    {
        if (errorText)
            *errorText = "Thong tin khong duoc de trong.";

        return false;
    }

    // Buoc 2: Kiem tra do dai username
    if (cleanUsername.length() > 50)
    {
        if (errorText)
            *errorText = "Username toi da 50 ky tu.";

        return false;
    }

    // Buoc 3: Kiem tra do dai display name
    if (cleanDisplayName.length() > 100)
    {
        if (errorText)
            *errorText = "Display name toi da 100 ky tu.";

        return false;
    }

    // Buoc 4: Kiem tra mat khau
    if (password.length() < 8)
    {
        if (errorText)
            *errorText = "Mat khau phai co it nhat 8 ky tu.";

        return false;
    }

    // Buoc 5: Tao Salt
    QString salt = createSalt();

    // Buoc 6: Bam mat khau
    QString passwordHash = hashPassword(password, salt);

    // Buoc 7: Luu vao MySQL
    QSqlQuery query = m_database.query(
        "INSERT INTO users "
        "(username, password_hash, password_salt, display_name) "
        "VALUES (?, ?, ?, ?)",
        {
            cleanUsername,
            passwordHash,
            salt,
            cleanDisplayName
        }
    );

    // Buoc 8: Kiem tra ket qua
    if (!query.isActive())
    {
        if (errorText)
            *errorText = m_database.lastError();

        return false;
    }

    if (errorText)
        errorText->clear();

    return true;
}