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
// Chuc nang 2: Dang nhap tai khoan
QVariantMap UserManager::authenticate(
    const QString &username,
    const QString &password,
    QString *errorText)
{
    // Buoc 1: Kiem tra du lieu dau vao
    const QString cleanUsername = username.trimmed();

    if (cleanUsername.isEmpty() || password.isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral(
                "Username va password khong duoc de trong."
            );
        }

        return {};
    }

    // Buoc 2: Tim tai khoan trong MySQL
    QSqlQuery query = m_database.query(
        QStringLiteral(
            "SELECT id, username, password_hash, "
            "password_salt, display_name, avatar "
            "FROM users WHERE username = ?"
        ),
        {cleanUsername}
    );

    // Buoc 3: Kiem tra loi truy van
    if (!query.isActive())
    {
        if (errorText)
        {
            *errorText = m_database.lastError();
        }

        return {};
    }

    // Buoc 4: Kiem tra username co ton tai khong
    if (!query.next())
    {
        if (errorText)
        {
            *errorText = QStringLiteral(
                "Sai username hoac password."
            );
        }

        return {};
    }

    // Buoc 5: Lay hash va salt tu database
    const QString expectedHash = query.value(2).toString();

    const QString salt = query.value(3).toString();

    // Buoc 6: Bam password nguoi dung nhap
    const QString actualHash = hashPassword(
        password,
        salt
    );

    // Buoc 7: So sanh hash
    if (actualHash != expectedHash)
    {
        if (errorText)
        {
            *errorText = QStringLiteral(
                "Sai username hoac password."
            );
        }

        return {};
    }

    // Buoc 8: Dang nhap thanh cong
    QVariantMap user;

    user.insert(
        QStringLiteral("id"),
        query.value(0)
    );

    user.insert(
        QStringLiteral("username"),
        query.value(1)
    );

    user.insert(
        QStringLiteral("display_name"),
        query.value(4)
    );

    user.insert(
        QStringLiteral("avatar"),
        query.value(5)
    );

    if (errorText)
    {
        errorText->clear();
    }

    return user;
    
}
// Chuc nang 3: Tim user theo username hoac display_name
bool UserManager::findUsers(
    const QString &keyword,
    QList<QVariantMap> *users,
    QString *errorText
)
{
    // Buoc 1: Kiem tra danh sach ket qua
    if (users == nullptr)
    {
        if (errorText)
        {
            *errorText = "Danh sach users khong hop le.";
        }

        return false;
    }

    users->clear();

    // Buoc 2: Kiem tra tu khoa
    const QString cleanKeyword = keyword.trimmed();

    if (cleanKeyword.isEmpty())
    {
        if (errorText)
        {
            *errorText = "Tu khoa tim kiem khong duoc de trong.";
        }

        return false;
    }

    // Buoc 3: Tao tu khoa LIKE
    const QString searchPattern =
        "%" + cleanKeyword + "%";

    // Buoc 4: Truy van MySQL
    QSqlQuery query = m_database.query(
        QStringLiteral(
            "SELECT id, username, display_name, created_at "
            "FROM users "
            "WHERE username LIKE ? "
            "OR display_name LIKE ? "
            "ORDER BY id ASC"
        ),
        {
            searchPattern,
            searchPattern
        }
    );

    // Buoc 5: Kiem tra loi truy van
    if (!query.isActive())
    {
        if (errorText)
        {
            *errorText = m_database.lastError();
        }

        return false;
    }

    // Buoc 6: Lay danh sach user
    while (query.next())
    {
        QVariantMap user;

        user.insert("id", query.value(0));
        user.insert("username", query.value(1));
        user.insert("display_name", query.value(2));
        user.insert("created_at", query.value(3));

        users->append(user);
    }

    // Buoc 7: Kiem tra ket qua
    if (users->isEmpty())
    {
        if (errorText)
        {
            *errorText = "Khong tim thay user.";
        }

        return true;
    }

    if (errorText)
    {
        errorText->clear();
    }

    return true;
}