#include "loginwindow.h"
#include "../register_window/registerwindow.h"
#include "../main_window/mainwindow.h"
#include "../../client/TCPClient.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

namespace {
void ensureLocalServerRunning()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/ChatServer.exe"),
        appDir + QStringLiteral("/../ChatServer.exe"),
        appDir + QStringLiteral("/ChatServer")
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            QProcess::startDetached(path, {});
            break;
        }
    }
}
}

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Chat TCP - Login");
    setMinimumSize(360, 260);
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("CHAT TCP");
    title->setAlignment(Qt::AlignCenter);
    auto *form = new QFormLayout;
    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Username");
    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    form->addRow("Username:", usernameInput);
    form->addRow("Password:", passwordInput);
    auto *loginButton = new QPushButton("Login", this);
    auto *registerButton = new QPushButton("Register", this);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(loginButton);
    layout->addWidget(registerButton);

    m_client = new TCPClient(this);
    ensureLocalServerRunning();
    connect(m_client, &TCPClient::responseReceived,
            this, &LoginWindow::handleServerResponse);
    connect(m_client, &TCPClient::connectionError, this, [this](const QString &err) {
        if (m_waitingLogin) {
            m_waitingLogin = false;
            QMessageBox::warning(this, "Connection Error", err);
        }
    });

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::login);
    connect(registerButton, &QPushButton::clicked, this, &LoginWindow::openRegister);
}

void LoginWindow::login()
{
    if (usernameInput->text().trimmed().isEmpty() || passwordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Login Error", "Please enter username and password.");
        return;
    }
    // TODO: Connect to NV4 TCP Client and authenticate with server.
    const QString user = usernameInput->text().trimmed();
    const QString pass = passwordInput->text();
    m_waitingLogin = true;

    if (!m_client->isConnected()) {
        ensureLocalServerRunning();
        QTimer::singleShot(250, this, [this, user, pass]() {
            QObject *ctx = new QObject(this);
            connect(m_client, &TCPClient::connected, ctx, [this, ctx, user, pass]() {
                ctx->deleteLater();
                if (m_waitingLogin)
                    m_client->login(user, pass);
            });
            m_client->connectToServer(QStringLiteral("127.0.0.1"), 5555);
        });
        return;
    }

    m_client->login(user, pass);
}

void LoginWindow::handleServerResponse(const QJsonObject &response)
{
    if (!m_waitingLogin)
        return;
    const QString reqType = response.value(QStringLiteral("request_type")).toString();
    if (!reqType.isEmpty() && reqType != QStringLiteral("login"))
        return;

    m_waitingLogin = false;
    if (!response.value(QStringLiteral("ok")).toBool()) {
        const QString err = response.value(QStringLiteral("error")).toString();
        QMessageBox::warning(this, "Login Error",
                             err.isEmpty() ? QStringLiteral("Sai thông tin đăng nhập.") : err);
        return;
    }

    const QJsonObject payload = response.value(QStringLiteral("payload")).toObject();
    QString userId = payload.value(QStringLiteral("user_id")).toString();
    if (userId.isEmpty())
        userId = QString::number(payload.value(QStringLiteral("id")).toInt());
    QString username = payload.value(QStringLiteral("username")).toString().trimmed();
    if (username.isEmpty())
        username = usernameInput->text().trimmed();

    m_client->setParent(nullptr);
    auto *mainWindow = new MainWindow(m_client, userId, username);
    m_client->setParent(mainWindow);
    mainWindow->show();
    close();
}

void LoginWindow::openRegister()
{
    auto *window = new RegisterWindow(m_client, nullptr);
    window->setWindowFlag(Qt::Window, true);
    window->setAttribute(Qt::WA_DeleteOnClose);
    this->hide();
    connect(window, &QObject::destroyed, this, [this]() {
        this->show();
        this->raise();
        this->activateWindow();
    });
    window->show();
}
